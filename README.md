# RLLager — Warehouse Simulation Environment

> **OBS: Detta är ett experimentprojekt som skrotades. Se avsnittet "Varför det dog" längst ner.**

---

## Vad var tanken?

Idén var att bygga en lager-simulering i C++ och koppla den till en Python-baserad Reinforcement Learning-agent. C++ hanterar den tunga simuleringen — robotrörelser, pathfinding, inventering, events — medan Python-sidan fattar beslut om vad robotarna ska göra härnäst.

Konceptet:
- Ett grafbaserat lager med noder (hyllor, laddstationer, lastkajer, frontdisk)
- Autonoma robotar med batteri, position och uppgiftskö
- Temperatorzoner (Hot/Warm/Cold) för produkthantering
- Kundorder, leveranser och restock-events som triggas under simuleringen
- En extern RL-agent som väljer vilken robot som gör vad

Varje episod simulerar en timme i lagret (3600 tidssteg à 1 sekund). Agenten får ett JSON-meddelande per event och svarar med ett beslut.

---

## Arkitektur

```
C++ Simulation (warehouse_sim)
        │
        │  JSON över stdout → named pipe
        ▼
  [cpp_to_py_pipe]
        │
        ▼
Python RL-Agent (rl_agent.py)
        │
        │  JSON över stdout → named pipe
        ▼
  [py_to_cpp_pipe]
        │
        ▼
C++ Simulation (läser stdin)
```

Kommunikationen sker via namngivna pipes (FIFOs) med JSON-meddelanden. C++ omdirigerar sin `stdout`/`stdin` till pipesen via `freopen()`. Python läser och skriver till samma pipes.

Meddelandetyper: `INIT`, `NEW_TASK`, `ROBOT_STATUS`, `ACTION_DECISION`, `WAIT_DECISION`, `EPISODE_END`, `RESET`

---

## Köra projektet (om du ändå vill)

```bash
make                          # Kompilera C++-simulationen
./run_simulation.sh -e 5      # Kör 5 episoder
./run_simulation.sh --debug   # Debug-output i terminalen
./run_simulation.sh -h        # Hjälp
```

Kräver: `g++` med C++17-stöd, Python 3, och att `nlohmann/json` (json.hpp) ligger under `includes/`.

---

## Varför det dog

**Kort svar:** C++–Python-kommunikationen via pipes är ett helvete.

**Längre svar:**

Idén med namngivna pipes verkade elegant på pappret — inga externa bibliotek, inget nätverk, bara ren IPC. I praktiken blev det en ständig kamp mot buffring, race conditions och odefinierat beteende vid episodgränser.

Konkreta problem:

- **Pipe-flushing och RESET**: Vid episodslut skickar C++ ett `EPISODE_END`-meddelande och väntar på ett `RESET` från Python. På grund av hur OS:et buffrar pipe-läsningar krävdes ibland 10+ försök innan RESET-meddelandet detekterades på C++-sidan — utan att något i koden förklarade varför.

- **Synkronisering vid uppstart**: Processerna startas parallellt i shell-scriptet (för att undvika deadlock på pipe-öppning), men det skapade en race condition där C++ ibland försökte skriva till pipen innan Python var redo att läsa.

- **`freopen()` + `ios_base::sync_with_stdio(false)`**: Att omdirigera `stdout`/`stdin` via `freopen()` och sedan stänga av C++-strömmens synkning är ett recept på subtila buggar. Buffring på C-nivå och C++-nivå konflikar och beteendet är plattformsberoende.

- **Debuggbarhet nära noll**: När något gick fel var det omöjligt att veta om problemet låg i C++-koden, Python-koden, pipe-buffringen, processhanteringen i bash-scriptet, eller någon kombination av dessa. `stderr` loggades till fil, `stdout` gick till pipen, och ibland kom meddelanden ur ordning.

**Lärdomen:** För den här typen av tight-loop simulation–agent-kommunikation är pipes fel verktyg. Bättre alternativ hade varit:
- Kompilera simuleringen som ett Python-extension (pybind11 / ctypes)
- Dela minne direkt via shared memory
- Köra allt i ett enda Python-process med Cython eller Numba för prestanda

Projektet lades ner när felsökningstiden för kommunikationsskiktet konsekvent överskred tiden som lades på faktisk RL-logik.

---

## Vad som faktiskt fungerade

Trots allt kom simuleringskärnan ganska långt:
- Dijkstra-pathfinding med trängselavoidance
- Batterikonsumtion och laddningslogik
- Produktpopularitet med decay och automatisk zonklassificering (Hot/Warm/Cold)
- Komplett event-system med slumpmässiga order och leveranser
- Logger med heatmaps och episodmetrics

RL-agenten i `rl_agent.py` är regelbaserad (inte tränad) — den kom aldrig längre än heuristisk uppgiftstilldelning.

---

*Experimentstatus: Avbrutet. Koden är bevarad som referens.*
