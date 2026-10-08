# Security and privacy

## Things that must never be committed or shared

| File | Contains | Protected by |
|---|---|---|
| `gateway/secret.env` | LLM / speech API keys, calendar link, admin token | `.gitignore` |
| `gateway/secrets/google_client.json` | Google OAuth client secret | `.gitignore` |
| `gateway/data/` | calendar login token, transcripts, voice recordings | `.gitignore` |
| `firmware/sdkconfig` | your Wi-Fi name and **password**, gateway address | `.gitignore` |
| `firmware/build/` and any `.bin` you build | the same Wi-Fi password, compiled in | `.gitignore` |
| `butterbot_stock_16MB.bin` (your backup) | your robot's factory image | `.gitignore` |

Before pushing a fork, run `git status` and check that none of these appear. If a key ever leaks, revoke it at the provider straight away. Deleting it from git history isn't enough.

## How the pieces are exposed

- **The gateway listens on TCP 8765 with no authentication on the voice connection.** Anything on your network can connect. Keep it on a trusted home network and **never forward port 8765 from the internet**.
- **Admin push** (`BB_ADMIN_TOKEN`) lets any program that knows the token make the robot speak or run a routine. Use a long random value. If it's unset, pushes are refused.
- **The robot** joins your Wi-Fi with credentials compiled into its firmware. Anyone with physical access to the robot and a USB cable can read its flash, Wi-Fi password included.
- **Calendar edits** need a spoken "yes", and only events ButterBot created can be moved or deleted. The Google token can still read every calendar it was granted, so keep `data/` private.

## What leaves your network

- **Audio:** none with local Whisper (the default). With a hosted speech service (Groq, OpenAI, Deepgram), each request's audio goes to that service.
- **Questions:** the transcript of anything that isn't a built-in command goes to your LLM provider, together with the last few exchanges and the current date and time. A local LLM (Ollama, LM Studio, vLLM) keeps this at home.
- **Calendar:** calendar requests go to Google, or to whoever hosts your iCal link.

## Reporting a vulnerability

Please open a private security advisory on this repository (*Security → Report a vulnerability*) rather than a public issue.
