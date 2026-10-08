# Calendar

ButterBot can read your calendar ("what does my day look like?"), add and change events after asking you to confirm, and remind you before meetings. All of it runs on the voice gateway; the robot firmware doesn't change.

## Pick a connection

| | iCal link | Google sign-in (OAuth) |
|---|---|---|
| Works with | Google, Outlook, iCloud, Fastmail, anything that publishes an `.ics` link | Google Calendar |
| Read your day | ✓ | ✓ |
| Ask about one named calendar | ✓ | ✓ (hidden calendars too) |
| Add / move / delete events by voice | ✗ | ✓, with a yes/no confirmation; only events ButterBot added can be moved or deleted |
| Meeting reminders | ✓ | ✓ |
| Setup | 2 minutes | 15 minutes: a Google Cloud project and one browser sign-in |
| Upkeep | none | sign in again every 7 days* |
| Security | anyone holding the link can read the calendar | access can be revoked any time in your Google account |

\* Google expires sign-ins after 7 days for apps that are still in "Testing" status. Leaving Testing requires a website and privacy policy on a domain you own.

---

## Option A: iCal link

1. **Copy the link.**
   - **Google Calendar** (on the web): ⚙ Settings → under *Settings for my calendars*, click the calendar → *Integrate calendar* → copy **Secret address in iCal format**.
   - **Outlook.com:** Settings → Calendar → Shared calendars → *Publish a calendar* → copy the ICS link.
   - **iCloud:** in the Calendar app, share the calendar as a *Public Calendar* and copy the link (change `webcal://` to `https://`).
2. **Add it to `gateway/secret.env`.** For several calendars, put the links on one line separated by spaces:
   ```
   BB_CALENDAR_ICS=https://calendar.google.com/calendar/ical/.../basic.ics
   ```
   `python setup.py` also asks for this, with hidden input.
3. **Restart the gateway.** With Docker, use `docker compose up -d`, not `restart`. The log should show `calendar: 1 iCal feed(s)`.

Treat the link like a password: anyone who has it can read that calendar.

---

## Option B: Google sign-in

### 1. Create the Google Cloud app (once)

In [Google Cloud Console](https://console.cloud.google.com), signed in with the Google account whose calendar ButterBot should use:

1. **Create a project.** Use the project picker at the top → *New project*, with any name (for example "ButterBot").
2. **Enable the Calendar API.** *APIs & Services → Library* → search **Google Calendar API** → *Enable*.
3. **Set up the consent screen** under *Google Auth Platform* (older consoles: *APIs & Services → OAuth consent screen*):
   - **Branding:** set an app name and your email as the support and developer-contact email. Leave the app domain, home page, privacy policy and authorized-domain fields **empty**.
   - **Audience:**
     - Choose user type **External**.
     - Under **Test users**, add your Google account. Without this, sign-in fails with *"Access blocked: … has not completed the Google verification process"*.
     - Leave the publishing status on **Testing**.
4. **Create the OAuth client.** *Clients → Create client* → type **Desktop app** → *Create* → **Download JSON**. Save it as `gateway/secrets/google_client.json`.

The JSON contains a client secret. It lives in `secrets/`, which `.gitignore` keeps out of git, so don't commit it or post it anywhere.

### 2. Sign in (on any computer with a browser)

From the `gateway` folder:

```
python calendar_auth.py --write secrets/google_client.json data/google_token.json
```

Leave out `--write` for read-only access. In the browser:

1. Choose your account.
2. On *"Google hasn't verified this app"*, click **Advanced → Go to <your app> (unsafe)**. This is expected: it's your own app, which Google hasn't reviewed.
3. **Leave both boxes ticked:** *See and download any calendar…* and *View and edit events…*. The edit permission alone can't list your calendars.
4. When the page says *"ButterBot: calendar access granted"*, close the tab.

<p align="center"><img src="images/calendar-signin.svg" alt="calendar_auth.py output listing the calendars the account can read" width="820"></p>

### 3. Point the gateway at the token

1. **Put `google_token.json` in the gateway's data folder.**
   - Docker, or a native install on Windows or macOS: `gateway/data/`
   - Linux native: `/opt/bb-voice-gateway/data/`
   - If you signed in on a different machine, copy the file over.
2. **Add this to `config.yaml`, then restart the gateway:**
   ```yaml
   calendar:
     provider: google
     token_file: google_token.json   # in the data folder
     calendars: selected             # the calendars ticked in Google Calendar | primary | [ids]
     write_calendar: primary         # where new events go unless you name a calendar
   ```

The log should show `calendar: Google Calendar (selected calendars, read/write)`.

### 4. Every 7 days: renew

ButterBot tells you when the sign-in has expired ("Your calendar login has expired…"). Renewing signs in again and puts the new token where the gateway reads it; **no restart is needed**:

| Gateway runs | Windows | Linux / macOS |
|---|---|---|
| on another machine | `powershell -ExecutionPolicy Bypass -File calendar_reauth.ps1 -GatewayHost user@<gateway-ip>` | `./calendar_reauth.sh user@<gateway-ip>` |
| on this machine | `… calendar_reauth.ps1 -GatewayHost local` | `./calendar_reauth.sh local` |

The remote option uses `ssh`/`scp`, so set up SSH access to the gateway machine first. To skip the host argument each time, set `BB_GATEWAY_SSH` once.

To revoke access at any time: Google Account → *Security* → *Third-party apps & services*.

---

## Using it

| Say | What happens |
|---|---|
| "What does my day look like?" / "Anything tomorrow?" | How many events, the first one, anything notable, your free time. Every event is read out only if there are three or fewer. |
| "What's on the book club calendar this week?" | Just that calendar. Names are matched loosely, so a misheard "book club" or "the book club calendar" still works. |
| "Add dentist Thursday at 2" | *"Add dentist, Thursday October 8 from 2 PM to 3 PM. Should I add it?"*: say **yes** within 6 s. Events are 1 hour unless you say otherwise. |
| "Add practice to the book club calendar Saturday at 9" | Same, on that calendar. Only calendars you own or can edit are allowed. |
| "Move the dentist to Friday at 3" / "Delete the dentist" | Same yes/no check. Works only on events ButterBot added; it says so if you ask about one of your own. |
| "Remind me 10 and 2 minutes before meetings" / "Turn off meeting reminders" | Changes meeting reminders. The change is remembered. |

Events ButterBot adds carry a private tag and the description "Added by ButterBot". That tag is how the gateway tells which events it may change.

**Meeting reminders:** before each timed event the robot says *"Heads up: Standup starts in 15 minutes, at 9 AM."* At 5 minutes it says *"Time to wrap up: …"*.
- All-day events, events you declined, and out-of-office or focus-time blocks are skipped.
- Events at the same time are combined into one sentence.
- Video-call links are read as "It's online".
- If the robot is off at reminder time, the reminder is retried until the meeting starts.

The reminder times are set by `calendar.remind_minutes_before` in `config.yaml` (default `[15, 5]`), or by voice.
