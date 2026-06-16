# Football 11v11

A playable top-down 11 vs 11 soccer game written in C++ with SFML.

## Features

- Full 11v11 matches with 4-4-2 formations
- Realistic pitch markings (penalty areas, center circle, goals)
- Control one player on the home team (Blue FC) vs AI opponents (Red United)
- Ball physics with passing, shooting, tackling, and sprinting
- AI teammates and opponents with positioning, marking, and decision-making
- Match timer (90-minute game compressed into ~9 minutes)
- Two halves with side switching at half time
- Goals, kickoffs, and celebrations

## Requirements

- macOS (or Linux/Windows with minor adjustments)
- C++17 compiler (Xcode Command Line Tools or Clang/GCC)
- CMake 3.16+
- Internet connection for first build (SFML is downloaded automatically)

## Build & Run

```bash
chmod +x build.sh
./build.sh
./build/Football11v11
```

Or manually:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./Football11v11
```

## Controls

| Key | Action |
|-----|--------|
| **WASD** / **Arrow Keys** | Move controlled player |
| **Shift** | Sprint |
| **Space** | Shoot (when you have the ball) |
| **E** | Pass to nearest teammate |
| **Q** | Switch to player nearest the ball |
| **C** | Tackle |
| **P** | Pause |
| **Esc** | Quit |

## Tips

- Use **Q** to switch to the player closest to the ball when defending.
- Pass with **E** when opponents close you down.
- Sprint drains stamina — use it wisely on counter-attacks.
- Your controlled player is highlighted with a yellow ring.

## Project Structure

```
FootBall/
├── CMakeLists.txt      # Build configuration (fetches SFML)
├── build.sh            # One-command build script
├── include/            # Headers
└── src/                # Source files
    ├── main.cpp
    ├── Game.cpp        # Main loop, match logic
    ├── Field.cpp       # Pitch rendering
    ├── Player.cpp      # Player movement & actions
    ├── Ball.cpp        # Ball physics
    ├── Team.cpp        # Formations & squads
    ├── AI.cpp          # AI behavior
    └── Renderer.cpp    # HUD & overlays
```

## License

MIT — feel free to modify and extend!
