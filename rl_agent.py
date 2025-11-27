#!/usr/bin/env python3
"""
Warehouse RL Agent
Communicates with C++ simulation via JSON over stdin/stdout
"""

import sys
import json
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from enum import Enum
import random

class TaskType(Enum):
    CUSTOMER_ORDER = "CUSTOMER_ORDER"
    INCOMING_DELIVERY = "INCOMING_DELIVERY"
    RESTOCK_REQUEST = "RESTOCK_REQUEST"

class ActionType(Enum):
    PICKUP_AND_DELIVER = "PICKUP_AND_DELIVER"
    RESTOCK = "RESTOCK"
    CHARGE = "CHARGE"
    HANDOVER = "HANDOVER"
    WAIT = "WAIT"

@dataclass
class Robot:
    id: str
    index: int
    current_node: int
    target_node: int
    battery: float
    status: str
    carrying: bool
    has_order: bool
    speed: float

@dataclass
class Task:
    task_id: str
    task_type: str
    product_id: int
    quantity: int
    source_node: int
    target_node: int
    priority: str
    deadline: float

class WarehouseRLAgent:
    """
    RL Agent for warehouse optimization.
    Makes decisions about robot task assignment and coordination.
    """
    
    def __init__(self):
        self.warehouse_layout = None
        self.products = []
        self.episode_count = 0
        self.task_count = 0
        self.decisions_made = 0
        
        # Learning parameters (simplified for now)
        self.exploration_rate = 0.2
        
    def log(self, message: str):
        """Log to stderr so it doesn't interfere with JSON communication."""
        sys.stderr.write(f"[RL] {message}\n")
        sys.stderr.flush()
    
    def send_message(self, msg: Dict):
        """Send JSON message to C++ sim."""
        print(json.dumps(msg), flush=True)
    
    def receive_message(self) -> Optional[Dict]:
        """Receive JSON message from C++ sim."""
        try:
            line = sys.stdin.readline()
            if not line:
                return None
            return json.loads(line.strip())
        except json.JSONDecodeError as e:
            self.log(f"JSON decode error: {e}")
            return None
    
    def send_ready(self):
        """Notify sim that we're ready."""
        self.send_message({"type": "READY"})
    
    def send_action(self, task_id: str, robot_index: int, 
                   action_type: ActionType, product_id: int = -1,
                   source_node: int = -1, target_node: int = -1):
        """Send action decision."""
        msg = {
            "type": "ACTION_DECISION",
            "task_id": task_id,
            "action": {
                "robot_index": robot_index,
                "action_type": action_type.value,
                "product_id": product_id,
                "source_node": source_node,
                "target_node": target_node,
                "strategy": "direct"
            }
        }
        self.send_message(msg)
        self.decisions_made += 1
    
    def send_wait(self, task_id: str, reason: str = "no_robots_available"):
        """Send wait decision."""
        msg = {
            "type": "WAIT_DECISION",
            "task_id": task_id,
            "reason": reason,
            "estimated_wait_time": 10.0
        }
        self.send_message(msg)
    
    def send_reset(self, episode_number: int):
        """Request reset for new episode."""
        msg = {
            "type": "RESET",
            "episode_number": episode_number
        }
        self.send_message(msg)
    
    def initialize(self):
        """Wait for INIT message and extract warehouse info."""
        self.log("Waiting for INIT message...")
        msg = self.receive_message()
        
        if not msg or msg.get("type") != "INIT":
            self.log("ERROR: Expected INIT message")
            return False
        
        self.warehouse_layout = msg.get("warehouse_layout", {})
        self.products = msg.get("products", [])
        robots_data = msg.get("robots", [])
        
        self.log(f"Initialized with {len(self.warehouse_layout.get('nodes', []))} nodes")
        self.log(f"  {len(self.products)} products")
        self.log(f"  {len(robots_data)} robots")
        
        # Send READY
        self.send_ready()
        
        return True
    
    def find_best_robot(self, task: Task, state: Dict) -> int:
        """
        Select best robot for the task.
        Strategy: Closest idle robot with enough battery.
        """
        robots = [Robot(**r) for r in state.get("robots", [])]
        
        best_robot_idx = -1
        best_score = float('-inf')
        
        for robot in robots:
            # Must be idle and have enough battery
            if robot.status != "Idle":
                continue
            
            if robot.battery < 30.0:
                continue
            
            # Simple scoring: higher battery = better
            # In real RL, this would be learned
            score = robot.battery
            
            # Bonus if already near the target
            if task.source_node >= 0:
                # Could calculate distance here
                pass
            
            if score > best_score:
                best_score = score
                best_robot_idx = robot.index
        
        return best_robot_idx
    
    def find_product_shelf(self, product_id: int, state: Dict) -> Tuple[int, int]:
        """Find which shelf has this product with stock."""
        inventory = state.get("inventory", [])
        
        best_node = -1
        best_slot = -1
        max_quantity = 0
        
        for shelf_data in inventory:
            node_idx = shelf_data.get("node_index", -1)
            slots = shelf_data.get("slots", [])
            
            for slot in slots:
                if slot.get("product_id") == product_id:
                    quantity = slot.get("occupied", 0)
                    if quantity > max_quantity:
                        max_quantity = quantity
                        best_node = node_idx
                        best_slot = slot.get("slot_index", -1)
        
        return best_node, best_slot
    
    def find_best_shelf_for_restock(self, product_id: int, state: Dict) -> int:
        """Find best shelf for restocking a product."""
        inventory = state.get("inventory", [])
        
        # Strategy: Find shelf with lowest fill rate for this product
        best_node = -1
        lowest_fill_rate = 1.0
        
        for shelf_data in inventory:
            zone = shelf_data.get("zone", "Cold")
            node_idx = shelf_data.get("node_index", -1)
            slots = shelf_data.get("slots", [])
            
            for slot in slots:
                if slot.get("product_id") == product_id:
                    fill_rate = slot.get("fill_rate", 1.0)
                    
                    # Prefer Hot/Warm zones for popular products
                    # (you could check product popularity here)
                    zone_bonus = {"Hot": 0.0, "Warm": 0.1, "Cold": 0.2}.get(zone, 0.3)
                    adjusted_fill_rate = fill_rate + zone_bonus
                    
                    if adjusted_fill_rate < lowest_fill_rate:
                        lowest_fill_rate = adjusted_fill_rate
                        best_node = node_idx
        
        return best_node
    
    def handle_customer_order(self, task: Task, state: Dict):
        """Handle customer order task."""
        self.log(f"Handling customer order: Product {task.product_id} x{task.quantity}")
        
        # Find robot
        robot_idx = self.find_best_robot(task, state)
        if robot_idx == -1:
            self.log("  No robot available, waiting...")
            self.send_wait(task.task_id, "no_robots_available")
            return
        
        # Find product location
        source_node, slot_idx = self.find_product_shelf(task.product_id, state)
        if source_node == -1:
            self.log(f"  Product {task.product_id} not found in inventory!")
            self.send_wait(task.task_id, "product_not_available")
            return
        
        # Send action
        self.log(f"  Assigning robot {robot_idx}: Pick from node {source_node} -> Deliver to {task.target_node}")
        self.send_action(
            task_id=task.task_id,
            robot_index=robot_idx,
            action_type=ActionType.PICKUP_AND_DELIVER,
            product_id=task.product_id,
            source_node=source_node,
            target_node=task.target_node
        )
    
    def handle_incoming_delivery(self, task: Task, state: Dict):
        """Handle incoming delivery task (restock from loading dock)."""
        self.log(f"Handling delivery: Product {task.product_id} x{task.quantity}")
        
        # Find robot
        robot_idx = self.find_best_robot(task, state)
        if robot_idx == -1:
            self.log("  No robot available, waiting...")
            self.send_wait(task.task_id, "no_robots_available")
            return
        
        # Find best shelf for this product
        target_shelf = self.find_best_shelf_for_restock(task.product_id, state)
        if target_shelf == -1:
            self.log(f"  No shelf found for product {task.product_id}")
            self.send_wait(task.task_id, "no_shelf_available")
            return
        
        # Send action
        self.log(f"  Assigning robot {robot_idx}: Restock from loading dock -> Shelf {target_shelf}")
        self.send_action(
            task_id=task.task_id,
            robot_index=robot_idx,
            action_type=ActionType.RESTOCK,
            product_id=task.product_id,
            source_node=task.source_node,
            target_node=target_shelf
        )
    
    def handle_restock_request(self, task: Task, state: Dict):
        """Handle low stock restock request."""
        self.log(f"Handling restock request: Product {task.product_id}")
        
        # Lower priority - could wait if robots busy
        robot_idx = self.find_best_robot(task, state)
        if robot_idx == -1:
            self.log("  No robot available for restock, deferring...")
            self.send_wait(task.task_id, "low_priority_deferred")
            return
        
        # Send restock action
        self.send_action(
            task_id=task.task_id,
            robot_index=robot_idx,
            action_type=ActionType.RESTOCK,
            product_id=task.product_id,
            source_node=task.source_node,
            target_node=task.target_node
        )
    
    def handle_task(self, task: Task, state: Dict):
        """Main task handler - routes to specific handlers."""
        self.task_count += 1
        
        task_type = TaskType(task.task_type)
        
        if task_type == TaskType.CUSTOMER_ORDER:
            self.handle_customer_order(task, state)
        elif task_type == TaskType.INCOMING_DELIVERY:
            self.handle_incoming_delivery(task, state)
        elif task_type == TaskType.RESTOCK_REQUEST:
            self.handle_restock_request(task, state)
        else:
            self.log(f"Unknown task type: {task.task_type}")
            self.send_wait(task.task_id, "unknown_task_type")
    
    def run(self):
        """Main agent loop."""
        if not self.initialize():
            return
        
        self.log("Entering main loop...")
        
        while True:
            msg = self.receive_message()
            
            if not msg:
                self.log("Connection closed")
                break
            
            msg_type = msg.get("type")
            
            if msg_type == "NEW_TASK":
                # New task to handle
                task_data = msg.get("task", {})
                state = msg.get("state", {})
                
                task = Task(
                    task_id=task_data.get("task_id", ""),
                    task_type=task_data.get("task_type", ""),
                    product_id=task_data.get("product_id", -1),
                    quantity=task_data.get("quantity", 1),
                    source_node=task_data.get("source_node", -1),
                    target_node=task_data.get("target_node", -1),
                    priority=task_data.get("priority", "normal"),
                    deadline=task_data.get("deadline", 0.0)
                )
                
                self.handle_task(task, state)
            
            elif msg_type == "ROBOT_STATUS":
                # Robot status update
                status_type = msg.get("status_type", "")
                robot_idx = msg.get("robot_index", -1)
                task_id = msg.get("task_id", "")
                
                self.log(f"Robot {robot_idx} status: {status_type} (task: {task_id})")
            
            elif msg_type == "ACK":
                # Acknowledgment
                task_id = msg.get("task_id", "")
                self.log(f"Task {task_id} acknowledged by sim")
            
            elif msg_type == "EPISODE_END":
                # Episode finished
                metrics = msg.get("metrics", {})
                self.log(f"Episode {self.episode_count} ended")
                self.log(f"  Orders completed: {metrics.get('orders_completed', 0)}")
                self.log(f"  Orders failed: {metrics.get('orders_failed', 0)}")
                self.log(f"  Decisions made: {self.decisions_made}")
                
                # Start new episode
                self.episode_count += 1
                self.task_count = 0
                self.decisions_made = 0
                
                self.log(f"Requesting episode {self.episode_count + 1}...")
                self.send_reset(self.episode_count + 1)
                
                # Wait for new INIT
                if not self.initialize():
                    break
            
            elif msg_type == "ERROR":
                error_msg = msg.get("message", "Unknown error")
                self.log(f"ERROR from sim: {error_msg}")
            
            else:
                self.log(f"Unknown message type: {msg_type}")


if __name__ == "__main__":
    agent = WarehouseRLAgent()
    
    try:
        agent.run()
    except KeyboardInterrupt:
        sys.stderr.write("\n[RL] Shutting down...\n")
    except Exception as e:
        sys.stderr.write(f"\n[RL] Fatal error: {e}\n")
        import traceback
        traceback.print_exc(file=sys.stderr)