# How BetterButterBot works

## The pieces

```mermaid
flowchart TB
    subgraph robot["ButterBot (ESP32-S3 firmware)"]
        mic["Microphone<br/>16 kHz"]
        ww["WakeWord service<br/>microWakeWord 'butterbot'"]
        ls["ListenState<br/>streams µ-law audio"]
        vl["VoiceLink<br/>Wi-Fi + WebSocket"]
        mn["On-device MultiNet<br/>(fallback)"]
        rt["Routines<br/>built-in scenarios · SpeakReply · Alarm · Find me"]
        portal_fw["Portal<br/>RC mode · motor task + stick watchdog<br/>status · battery · Wi-Fi"]
        cam["CameraStream<br/>320×240, JPEG made on the robot"]
        tts["flite speech"]
        ble["Bluetooth<br/>phone app · RC controller"]
    end
    subgraph gw["Voice gateway (Python)"]
        ep["Endpointer<br/>Silero speech detector"]
        stt["Speech-to-text<br/>faster-whisper / hosted"]
        im["Intent matcher<br/>238 command phrases"]
        chat["LLM client<br/>OpenAI-compatible / Anthropic"]
        agent["Agent<br/>timers · alarms · reminders<br/>calendar · meeting reminders"]
        viewer["Explorer portal<br/>sign-in page, :8766"]
        db[("data/log.sqlite3")]
    end
    mic --> ww --> ls
    ls --> vl
    ls -. gateway unreachable .-> mn --> rt
    vl <== "ws://gateway:8765/bb" ==> ep
    ep --> stt --> im
    im -- "command" --> vl
    im -- "anything else" --> chat <--> agent
    agent --> db
    agent -- "push: say / alarm / routine" --> vl
    vl --> rt --> tts
    cam -- "JPEG frames (same socket)" --> vl
    vl -- frames --> viewer
    viewer -- "mode · drive · say · listen" --> vl
    vl <--> portal_fw
    viewer <-- "MJPEG + WebSocket" --> browser([Browser on your network])
```

## A question, start to finish

```mermaid
sequenceDiagram
    autonumber
    participant U as You
    participant R as ButterBot
    participant G as Gateway
    participant L as LLM
    U->>R: "Butterbot"
    Note over R: wake word detected on the robot
    R->>G: {"type":"start"} + audio frames (µ-law, ~16 KB/s)
    U->>R: "what's the capital of Australia?"
    G-->>R: {"type":"endpoint"}: you stopped talking (Silero)
    Note over G: Whisper → "what's the capital of Australia"<br/>not one of the 238 commands
    G->>L: question + short history + tools
    L-->>G: "Canberra. ..."
    G-->>R: {"type":"result", "reply": "Canberra. ..."}
    R->>U: speaks the answer (flite)
    Note over R: listens 3 s for a follow-up
```

Built-in commands skip the LLM: the gateway returns the command's index, and the robot runs the same routine its on-device recognition would have triggered. If the gateway can't be reached, or speaks a different command list (checked with a vocabulary hash at connect), the robot falls back to its on-device MultiNet recognition.

## The robot's states

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Listen: "Butterbot" / button / app
    Idle --> Scenario: gateway push (timer, alarm, reminder, meeting reminder)
    Listen --> Scenario: command or answer
    Listen --> Idle: nothing heard / "never mind"
    Scenario --> Listen: answer given → follow-up window
    Scenario --> Idle: routine finished
    Listen --> Listen: "Butterbot" during follow-up → fresh listen
    Idle --> RC: controller takes over
    RC --> Idle
    Idle --> Idle: portal RC on/off (Idle runs no routines, ignores the wake word)
```

- **Wake word:** runs only in Idle. During a follow-up window, the listening audio is fed to it as well, so saying "Butterbot" starts a fresh question in under a second.
- **Pushes from the gateway** wait until the robot is idle, so an alarm never cuts into a conversation.
- **What each push does:** alarms sing The Butter Song, timers sing The Toast Song, reminders are spoken, and "find me" starts the controller's Summon routine. At connect the robot lists which songs and routines its firmware has, so the gateway falls back to speech (or says it can't) for older firmware.

## The camera stream

- **One camera, one owner at a time.** Face detection and object recognition use small 128×128 RGB frames; the stream switches the camera to 320×240 raw frames, which the robot compresses to JPEG itself (about 2–3 frames a second). A routine that needs the camera (find me, face recognition, object recognition) claims it for its whole run, from loading its model to unloading it: the stream pauses once, the routine has the camera to itself, and the stream resumes afterwards if it's still wanted.
- **Long-range find me.** The face detector shrinks every frame to ~160×120, so its range is set by that size, not the camera's. Find me switches the camera to 320×240 and also runs the detector on three full-detail 160×120 crops, which doubles the size faces appear at. Owner recognition keeps the 128×128 path its faces were registered with.
- **Why not the camera's own JPEG?** On this robot it doesn't hold up: at 640×480 frames arrived truncated, and at 320×240 the driver dropped nearly every frame for a missing end marker, even with a slower camera clock. The robot's own compression is what CircuitMess's RC feed uses too.
- **Frames ride the voice socket** as a `{"type":"frame"}` header plus a binary message, so the robot never listens on the network. Frames pause while you're talking to it, and the resolution steps down on its own if Wi-Fi can't keep up or frames arrive incomplete.
- **The gateway serves the portal** behind a sign-in (scrypt-hashed passwords, HttpOnly session cookie, lockout after repeated failures). The stream is never restored: it is off after the robot or the gateway restarts, and when the robot's connection drops.

## Explorer mode

```mermaid
sequenceDiagram
    participant B as Browser (portal)
    participant G as Gateway
    participant R as ButterBot
    B->>G: {"type":"mode","mode":"rc"} (one driver at a time)
    G->>R: {"type":"mode","rc":true}
    R-->>G: {"type":"status","mode":"rc",...}
    Note over R: floor/front sensors off, idle routines and wake word off
    loop every 100 ms while the stick or keys are held
        B->>G: {"type":"drive","x":0.5,"y":1}
        G->>R: {"type":"drive","l":100,"r":33}
    end
    Note over R: no drive for 400 ms: motors stop
    B->>G: {"type":"mode","mode":"vc"} / sign-out / page gone 30 s
    G->>R: {"type":"mode","rc":false}
```

- **The gateway mixes the stick.** Stick position becomes track speeds (forward plus turn, scaled together so a diagonal keeps its direction) after a round dead zone; a push within a few degrees of an axis goes exactly along it. Only the driver's messages reach the robot.
- **Motors on their own task.** In RC the motors, the sensor switch and the 400 ms stick watchdog run on a task of their own on the robot, not on the network task, so a stalled connection can't keep the robot driving.
- **No restrictions in RC.** The floor and front sensors are switched off on the base board for the whole of RC and back on (with a fresh reading) when it ends; charging doesn't block driving either. The robot's button, a dropped link or a restart always return it to VC.
- **Chat.** Typed text is a `say` push, spoken word for word. Listen is a `listen` push: the robot streams once with `"chat":true` and the gateway returns the transcript to the page that asked, without matching commands or asking the LLM. The robot reports `speaking` / `listening` so the page never listens while it talks.
- **Battery and Wi-Fi.** The robot reports its level (taken only after 5 s parked, because the motors dip the voltage; not measurable while charging) and RSSI after each connect, every minute, every 5 s in RC, and when the charger is plugged in or out.

## Changing your calendar by voice

```mermaid
sequenceDiagram
    participant U as You
    participant R as ButterBot
    participant G as Gateway
    participant C as Google Calendar
    U->>R: "Add dentist Thursday at 2"
    R->>G: audio
    G->>G: LLM calls add_calendar_event → saved as a draft only
    G-->>R: "Add dentist, Thursday October 8 from 2 PM to 3 PM. Should I add it?"
    R->>U: asks (6 s to answer)
    U->>R: "Yes"
    R->>G: audio
    G->>C: create event (tagged "added by ButterBot")
    G-->>R: "Done. I added dentist ... to your calendar."
```

- **Moves and deletes** follow the same draft-then-confirm path. The gateway refuses them for any event without ButterBot's private tag, so events you created yourself can't be changed by voice.
- **Drafts expire** after 2 minutes.

## Network design notes

- **One radio, shared.** The ESP32-S3 shares a single radio between Wi-Fi and Bluetooth. Constant Bluetooth advertising was found to stall the Wi-Fi link for 10–15 s at a time. So while the gateway link is up, the firmware **pauses Bluetooth advertising** (`CONFIG_BB_BLE_QUIET_ON_LAN`). Devices already connected stay connected.
- **Separate send and receive locks.** The WebSocket client sends and receives under separate locks (`CONFIG_ESP_WS_CLIENT_SEPARATE_TX_LOCK`), so a slow audio upload can't block the gateway's replies.
- **Enough receive buffers for explorer mode.** With the camera sending frames and the joystick streaming commands in, the original 2 Wi-Fi receive buffers ran out and wedged the link until a restart. The firmware now allows 8 (`CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM`); they are only allocated while packets arrive.
- **Self-healing connection.** A watchdog rebuilds the WebSocket client if Wi-Fi is up but the gateway has been unreachable for 20 s, or right after Wi-Fi rejoins.
- **Sustained sensor readings.** The treads' speed difference makes the robot bounce, so the firmware only passes on a "floor lost" after 4 s and a "something close" after 0.5 s without a break (`FloorLostConfirmMs`, `FrontCloseConfirmMs` in `BaseBoard.h`). A return to normal goes through at once. Start checks need 3 unsafe readings in a row, and "upright" averages 8 accelerometer samples (the speaker and leftover bounce shook single samples). A real lift keeps the floor lost and still stops it.
- **Internal RAM is tight** (about 30 KB free while running). Audio buffers and task stacks live in PSRAM. Speech recognition on the robot (MultiNet) needs Wi-Fi's memory, so the robot drops Wi-Fi while it uses on-device recognition.

## Flash layout

| Partition | Offset | Size | Contents |
|---|---|---|---|
| nvs | 0x9000 | 24 KB | settings |
| factory | 0x10000 | 8880 KB | the firmware (stock was 99.6 % full; grown to fit) |
| model | 0x8bc000 | 2990 KB | on-device speech models (srmodels) |
| storage | 0xba8000 | 212 KB | sound clips, including The Butter Song and The Toast Song (about 2 KB left) |
| human_face_det / feat | 0xbdd000 / 0xc0f000 | 200 KB / 1265 KB | face detection models |
| objdet | 0xd4c000 | 2660 KB | object detection model |
| wakeword | 0xfe5000 | 64 KB | the "butterbot" model (new) |
