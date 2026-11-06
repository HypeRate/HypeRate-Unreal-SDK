# Fab listing draft

## Title

HypeRate for Unreal Engine

## Short description

Bring live heart rate into Unreal Engine with Blueprint and C++ events,
automatic WebSocket reconnects, simulation, Editor setup, and a runtime widget.

## Long description

HypeRate for Unreal Engine turns live BPM into a game-ready signal. Add one
Actor Component, bind a Blueprint event, and drive difficulty, horror systems,
fitness mechanics, animation, lighting, audio, VFX, UI, or streaming
experiences. The plugin handles the HypeRate WebSocket protocol, Phoenix channel
join/leave, heartbeats, game-thread dispatch, disconnects, and capped
exponential reconnects.

Develop without a wearable using four simulation profiles. Display BPM with the
included native UMG/Slate widget and compact history graph. Local test
credentials stay in per-user Editor settings and are excluded from packaged
builds.

## Key features

- Blueprint-ready HypeRate Client Actor Component
- Full C++ API and dynamic multicast events
- Live, connecting, reconnecting, simulation, error, and offline states
- Automatic Phoenix heartbeats and reconnects
- Resting, Workout, Intense, and Custom simulator
- Native BPM widget with history graph
- Local Editor setup and runtime credential workflow
- Protocol and simulation automation tests
- No bundled third-party code or binary runtime dependencies

## Technical information

- Distribution method: Code Plugin
- Verified engine support: Unreal Engine 5.8
- Modules: `HypeRate` (Runtime), `HypeRateEditor` (Editor)
- Dependencies: Unreal Engine WebSockets, JSON, UMG, Slate
- Network: outbound secure WebSocket to the configured HypeRate endpoint
- Platforms: source-compatible where Unreal's WebSockets module is supported;
  precompiled packages are supplied per reviewed engine/platform combination
- Documentation language: English
- Third-party service: HypeRate API access and a WebSocket token are required
- Third-party code/assets: none bundled

## Tags and search keywords

Recommended Fab tags and listing keywords:

- Unreal Engine
- Blueprint
- C++
- Code Plugin
- Heart Rate
- Biometrics
- WebSocket
- Gameplay
- Fitness
- Streaming
- UI
- Simulation
- Realtime
- Wearable
- HypeRate

## Important disclosure

The plugin itself is MIT-licensed. Access to the hosted HypeRate API is a
separate service and may have project-specific availability or commercial
terms. Buyers should review current terms at https://hyperate.io/api before use.

Heart-rate data is sensitive biometric information and is not medical data or
medical advice. Developers are responsible for consent, privacy, retention,
security, and appropriate credential handling in their experience.

## Media checklist

- 1920x1080 cover image, under 3 MB
- 1920x1080 Blueprint setup screenshot
- Editor setup screenshot: `Docs/images/hyperate-editor-setup.png`
- Widget simulation screenshot: `Docs/images/heart-rate-widget-simulation.png`
- 1920x1080 live wearable state screenshot
- Optional short 1920x1080 demo video

