# Plan: BMO Voice — Migrate TTS from Gemini to Xiaomi MiMo V2.5

> **STATUS: COMPLETE & VERIFIED (2026-06-01).** All 11 implementation tasks
> (T1–T11) are committed (`dcd7d1d` → `84c466f`), plus post-plan asset
> refinements and a bilingual STT prompt fix (`d1b1362`). Final verification
> wave passed:
> - `python -m pytest -q` → **319 passed**.
> - AR7 hermeticity (`test_audio_fake_hermeticity.py`) → 3 passed.
> - 10 firmware WAVs present, all valid `RIFF` 24kHz headers.
> - `app/agent/runtime.py` tool-calling logic **untouched** (scope held).
> - No `tp-` API key literal in any tracked file; `openai>=1.50.0` added;
>   `apiplayground.py` reads `MIMO_API_KEY` from env.

## TL;DR

> Replace the TTS layer only with `mimo-v2.5-tts-voiceclone` (BMO voice clone).
> Agent runtime (Google ADK + Gemini) and STT (Gemini) stay unchanged.
> Director emotion via zero-latency intent-mapping from existing `audio_code`.
> Responses become English-only. Regenerate 10 firmware WAVs in English.

**Deliverables**
- `app/audio/tts_mimo.py` (deferred `openai` import, AR7-safe)
- `synthesize_text(text, *, director=None, tag=None)` backward-compatible seam
- `DIRECTOR_MAP` (expanded emotion set keyed by `audio_code`)
- English-only INSTRUCTION + fake agent + dev-agent parity
- Updated tests (ID → EN assertions)
- Fixed `generate_firmware_sounds.py` (path bug + EN phrases + mimo provider)
- 10 regenerated WAVs in `firmware/Lyla-Taskbot/sd_template/sounds/`

**Effort**: Large | **Parallel**: YES (5 waves) | **Critical path**: seam → provider → wiring → assets → verify

---

## Context

### Original Request
Migrate Taskbot/BMO TTS to `mimo-v2.5-tts-voiceclone` to mimic the BMO persona, hitting the Xiaomi MiMo API. Two stages: integration, then static asset regeneration.

### Confirmed Decisions
- Director = zero-latency intent-mapping, expanded emotion set.
- Static assets = English.
- Replace Gemini TTS entirely (mimo default).
- API key via provided value, stored as `MIMO_API_KEY` in `.env`.
- OK to update Indonesian-assertion tests to English.

### Key Technical Facts (verified)
- MiMo = OpenAI-compatible `chat.completions`. Token-plan key → base_url `https://token-plan-sgp.xiaomimimo.com/v1`.
- Dual-role: `user` = director; `assistant` = dialogue + audio tags. Voice sample = base64 data URI in `audio.voice` (≤10MB).
- Output 24kHz mono WAV — matches existing path; firmware audio unchanged.
- V2.5 streaming = compatibility mode (full result only) → NO chunk streaming.
- AR7 (Hard Rule 8): `tts.py`/`_seam.py` no SDK import; provider SDK only via deferred import in method body.
- Reminder path: `reminder_tts.synthesize_for_reminder` has only `reminder.title`, no `audio_code` → needs a default reminder director.
- Hard Rule 5: no `output_schema`. Hard Rule 7: dev-agent parity enforced.

---

## Work Objectives

### Core Objective
Swap TTS backend to MiMo voice-clone with emotion-aware director, English-only replies, English static assets — without touching agent runtime or STT.

### Must Have
- `AUDIO_TTS_MODE=mimo` produces BMO-voiced 24kHz WAV with valid RIFF header.
- Director chosen from `audio_code` (live path) and a default (reminder path).
- English-only agent replies; fake agent routes ID+EN keywords.
- 10 English WAVs in the correct firmware path.
- `python -m pytest -q` green; AR7 hermeticity test passes.

### Must NOT Have (Guardrails)
- NO change to `app/agent/runtime.py` tool-calling logic or STT.
- NO `openai` import at module level in `app/audio/` (AR7).
- NO chunk-streaming implementation (zero benefit).
- NO hardcoded API key in any tracked file.
- NO `output_schema` on the Agent.
- NO new latency on the live reply path beyond the single TTS call.

---

## Verification Strategy

> Zero human intervention. All checks agent-executed.

- Unit/integration: pytest (existing framework). Update ID→EN assertions.
- Hermeticity: `app/tests/test_audio_fake_hermeticity.py` must pass.
- Manual QA: Bash one-shot synth via mimo mode → assert RIFF header + non-empty bytes; curl `/reminders/{id}/tts` → 200 + audio.
- Evidence → `.sisyphus/evidence/`.

---

## Execution Strategy

```
Wave 1 (foundation, parallel):
├── T1 Config keys (mimo_*) + .env.example
├── T2 Secure example key (rotate note + .env ref)
└── T3 DIRECTOR_MAP module (audio_code → director+tag)

Wave 2 (provider + seam):
├── T4 tts_mimo.py provider (deferred openai, base64 cache)
└── T5 Extend synthesize_text signature (director/tag) + mimo branch

Wave 3 (wiring):
├── T6 Live path: pass audio_code→director into TTS
├── T7 Reminder path: default reminder director
└── T8 English-only INSTRUCTION + fake agent ID+EN + dev parity

Wave 4 (tests + assets):
├── T9 Update ID→EN assertion tests
├── T10 Fix generate_firmware_sounds.py (path+EN+mimo)
└── T11 Regenerate 10 WAVs

Wave FINAL:
├── F1 Plan compliance (oracle)
├── F2 Code quality + AR7 (unspecified-high)
├── F3 Manual QA synth + reminder fetch (unspecified-high)
└── F4 Scope fidelity (deep)
```

Critical path: T1 → T5 → T4 → T6 → T9 → F1-F4

---

## TODOs

- [ ] 1. Add MiMo config keys + .env.example

  **What to do**:
  - In `app/config.py` add settings: `mimo_api_key: str = ""`, `mimo_base_url: str = "https://token-plan-sgp.xiaomimimo.com/v1"`, `mimo_model: str = "mimo-v2.5-tts-voiceclone"`, `mimo_voice_sample_path: str = "voice_clone_example/bmo_voice_sample.mp3"`.
  - Add the same keys to `.env.example` with empty/default values and a short comment.
  - Do NOT remove existing `audio_tts_*` keys.

  **Must NOT do**: Hardcode the real key in `config.py` or `.env.example`.

  **Recommended Agent Profile**: Category `quick`; Skills `[]`.

  **Parallelization**: Wave 1; Blocks T4, T5; Blocked By none.

  **References**:
  - `app/config.py` — existing pydantic-settings pattern (`audio_tts_mode`, `google_api_key`).
  - `.env.example` — keep in sync (AGENTS.md soft convention).

  **Acceptance Criteria**:
  - [ ] `python -c "from app.config import settings; print(settings.mimo_base_url)"` prints the token-plan URL.
  - [ ] `findstr /C:"MIMO_API_KEY" .env.example` returns a match.

  **QA Scenarios**:
  ```
  Scenario: settings load
    Tool: Bash
    Steps: 1. run python -c import settings; print mimo_model
    Expected: prints "mimo-v2.5-tts-voiceclone"
    Evidence: .sisyphus/evidence/task-1-settings.txt
  ```

  **Commit**: YES — `chore: add MiMo TTS config keys`

- [ ] 2. Secure the example API key

  **What to do**:
  - Edit `voice_clone_example/apiplayground.py` to read key from `os.environ["MIMO_API_KEY"]` instead of the hardcoded literal.
  - Add a top-of-file comment instructing to set `MIMO_API_KEY` in `.env` and to ROTATE the previously-committed key.

  **Must NOT do**: Leave any `tp-...` literal in the file.

  **Recommended Agent Profile**: Category `quick`; Skills `[]`.

  **Parallelization**: Wave 1; Blocks none; Blocked By none.

  **References**:
  - `voice_clone_example/apiplayground.py:15` — current hardcoded key (remove).

  **Acceptance Criteria**:
  - [ ] `findstr /R "tp-" voice_clone_example/apiplayground.py` returns NO match.

  **QA Scenarios**:
  ```
  Scenario: no literal key
    Tool: Bash
    Steps: 1. grep for tp- in the file
    Expected: zero matches
    Evidence: .sisyphus/evidence/task-2-keyscan.txt
  ```

  **Commit**: YES — `security: read MiMo key from env in example`

- [ ] 3. DIRECTOR_MAP module (expanded emotion set)

  **What to do**:
  - Create `app/audio/director_map.py` with `DIRECTOR_MAP: dict[str, tuple[str, str]]` mapping each `audio_code` to `(director_instruction, audio_tag_prefix)`.
  - Cover existing codes: `ok_task`, `ok_expense`, `ok_reminder`, `ok_summary`, `ok_generic`, `err_generic`, `fallback_tts`.
  - Add expanded-nuance codes so BMO is less monotone (e.g. `ok_task_excited`, `reminder_due`, `greeting`, `thinking`) — each a distinct BMO-persona director line + optional tag like `(sigh)`, `(giggle)`, `(curious)`.
  - Provide `DEFAULT = ("Friendly, warm BMO robot voice.", "")` and a `resolve(audio_code) -> tuple[str,str]` helper that falls back to DEFAULT.
  - Pure data + one function. NO SDK imports (AR7-safe; this file is in `app/audio/`).

  **Must NOT do**: Import `openai`/`google.adk` here.

  **Recommended Agent Profile**: Category `unspecified-low`; Skills `[]`.

  **Parallelization**: Wave 1; Blocks T5, T6, T7; Blocked By none.

  **References**:
  - `app/api/_audio_directive.py` — source of `audio_code` values (read to enumerate the canonical set).
  - `firmware/.../directive_dispatcher.cpp:14-23` — `parse_audio_code` lists every code the firmware understands; keep map keys aligned.

  **Acceptance Criteria**:
  - [ ] `python -c "from app.audio.director_map import resolve; print(resolve('err_generic'))"` returns a tuple with a non-empty director and a `(sigh)`-style tag.
  - [ ] `resolve('unknown_code')` returns DEFAULT.

  **QA Scenarios**:
  ```
  Scenario: map resolves + fallback
    Tool: Bash
    Steps: 1. resolve known code 2. resolve unknown code
    Expected: known→specific tuple; unknown→DEFAULT
    Evidence: .sisyphus/evidence/task-3-director.txt
  ```

  **Commit**: YES — `feat: add audio_code director map for MiMo emotion`

- [ ] 4. MiMo TTS provider (`app/audio/tts_mimo.py`)

  **What to do**:
  - Create `MimoTtsProvider` mirroring `tts_gemini.py` structure. Constructor takes `api_key`, `base_url`, `model`, `voice_sample_path`.
  - At init: read the voice sample file, base64-encode ONCE, cache as `self._voice_data_uri = f"data:audio/mpeg;base64,{b64}"`. Do NOT re-encode per call (latency/memory).
  - `synthesize(text, director=None, tag=None) -> SynthesisResult`: build messages `[{role:user, content:director or ""}, {role:assistant, content:(tag or "")+text}]`, call `chat.completions.create(model=..., messages=..., audio={"format":"wav","voice":self._voice_data_uri})`, base64-decode `message.audio.data`, return `SynthesisResult(mode="mimo", content_type="audio/wav", audio_bytes=..., text=text, metadata={...})`.
  - **DEFERRED import**: `from openai import OpenAI` INSIDE the method/constructor body, never at module top (AR7).
  - Use non-streaming (streaming has zero benefit). Raise `ConfigurationError` on empty `mimo_api_key`.

  **Must NOT do**: Module-level `import openai`. Streaming. Re-encode sample per call.

  **Recommended Agent Profile**: Category `deep`; Skills `[]`.

  **Parallelization**: Wave 2; Blocks T6, T7; Blocked By T1, T3.

  **References**:
  - `app/audio/tts_gemini.py` — provider class shape + deferred-import pattern to copy exactly.
  - `app/audio/_seam.py` — `SynthesisResult`/`ConfigurationError` contract.
  - `voice_clone_example/apiplayground.py:33-52` — exact MiMo request shape (model, messages, audio.voice data URI).

  **Acceptance Criteria**:
  - [ ] `app/tests/test_audio_fake_hermeticity.py` still passes (no SDK leak).
  - [ ] Bash one-shot (with real key) returns bytes starting with `RIFF`.

  **QA Scenarios**:
  ```
  Scenario: real synth returns RIFF wav
    Tool: Bash
    Preconditions: MIMO_API_KEY set
    Steps: 1. import MimoTtsProvider, synthesize("Hello, I am BMO", director="Cheerful BMO robot")
           2. assert audio_bytes[:4]==b"RIFF" and len>1000
    Expected: valid wav bytes
    Evidence: .sisyphus/evidence/task-4-mimo-synth.wav

  Scenario: empty key fails gracefully
    Tool: Bash
    Steps: 1. construct provider with empty key 2. expect ConfigurationError
    Expected: ConfigurationError raised, no crash
    Evidence: .sisyphus/evidence/task-4-error.txt
  ```

  **Commit**: YES — `feat: add MiMo voice-clone TTS provider`

- [ ] 5. Extend seam signature + mimo branch (`app/audio/tts.py`)

  **What to do**:
  - Change `synthesize_text(text)` → `synthesize_text(text, *, director: str | None = None, tag: str | None = None)`.
  - Keep `fake` and `gemini` branches ignoring `director`/`tag` (backward-compatible).
  - Add `mimo` branch: lazily construct + cache `MimoTtsProvider` (lock pattern like `_get_gemini_provider`), call `provider.synthesize(text, director=director, tag=tag)`.
  - Update `ConfigurationError` message to list `fake`, `gemini`, `mimo`.
  - This file MUST stay SDK-free (AR7) — provider import is inside the `mimo` branch via `app.audio.tts_mimo` (which itself defers openai).

  **Must NOT do**: Import `openai` in `tts.py`. Break existing single-arg callers (default args handle it).

  **Recommended Agent Profile**: Category `deep`; Skills `[]`.

  **Parallelization**: Wave 2; Blocks T6, T7; Blocked By T1, T3, T4.

  **References**:
  - `app/audio/tts.py:40-58` — current `synthesize_text` + `_get_gemini_provider` lock pattern.

  **Acceptance Criteria**:
  - [ ] Existing callers `synthesize_text("x")` still work (no TypeError).
  - [ ] `AUDIO_TTS_MODE=mimo` routes to MimoTtsProvider.
  - [ ] AR7 hermeticity test passes.

  **QA Scenarios**:
  ```
  Scenario: backward-compatible call
    Tool: Bash
    Steps: 1. AUDIO_TTS_MODE=fake; synthesize_text("hi") 2. assert SynthesisResult
    Expected: works, mode=fake
    Evidence: .sisyphus/evidence/task-5-comithpat.txt
  ```

  **Commit**: YES — `feat: route TTS seam to MiMo with director/tag`

- [ ] 6. Wire live audio path: audio_code → director

  **What to do**:
  - In the live reply path (where `synthesize_text` is called after the directive classifier produces `audio_code`), resolve `(director, tag) = director_map.resolve(audio_code)` and pass them into `synthesize_text(text, director=director, tag=tag)`.
  - Trace the exact call site: `app/api/_agent_helpers.py` / `app/api/audio.py` TTS step. Confirm `audio_code` is in scope at the synth call; if computed later, reorder minimally so it is available.

  **Must NOT do**: Change STT, agent runtime, or the directive classifier logic. Add LLM calls.

  **Recommended Agent Profile**: Category `deep`; Skills `[]`.

  **Parallelization**: Wave 3; Blocks T9; Blocked By T3, T5.

  **References**:
  - `app/api/audio.py` + `app/api/_agent_helpers.py` — find the `synthesize_text`/TTS invocation and the `audio_code` source.
  - `app/api/_audio_directive.py` — how `audio_code` is produced.

  **Acceptance Criteria**:
  - [ ] `err_generic` reply produces a request whose `user` message contains the apologetic director (verify via unit test mocking the provider).
  - [ ] AR7 + full pytest green.

  **QA Scenarios**:
  ```
  Scenario: director threaded on live path
    Tool: Bash
    Steps: 1. mock MimoTtsProvider.synthesize, run audio reply with audio_code=ok_task
           2. assert director arg == DIRECTOR_MAP[ok_task][0]
    Expected: correct director passed
    Evidence: .sisyphus/evidence/task-6-live-director.txt
  ```

  **Commit**: YES — `feat: pass audio_code director into live TTS`

- [ ] 7. Wire reminder path: default reminder director

  **What to do**:
  - In `app/audio/reminder_tts.py::synthesize_for_reminder`, resolve a director for reminders. Since reminders have no `audio_code`, use a dedicated key (e.g. `reminder_due`) from DIRECTOR_MAP → warm, gentle reminder director.
  - Pass `director`/`tag` into `synthesize_text(reminder.title, director=..., tag=...)`.

  **Must NOT do**: Change reminder scheduling/dispatch logic or cache key shape.

  **Recommended Agent Profile**: Category `unspecified-high`; Skills `[]`.

  **Parallelization**: Wave 3; Blocks T9; Blocked By T3, T5.

  **References**:
  - `app/audio/reminder_tts.py:36-87` — `synthesize_for_reminder`, the `synthesize_text(reminder.title)` call.
  - `app/audio/director_map.py` — `reminder_due` key (added in T3).

  **Acceptance Criteria**:
  - [ ] Reminder synth passes the reminder director (unit test with mocked provider).
  - [ ] `tts_status` still transitions to `ready` on success.

  **QA Scenarios**:
  ```
  Scenario: reminder uses reminder director
    Tool: Bash
    Steps: 1. mock provider 2. synthesize_for_reminder 3. assert director == reminder_due line
    Expected: reminder director passed, tts_status=ready
    Evidence: .sisyphus/evidence/task-7-reminder-director.txt
  ```

  **Commit**: YES — `feat: reminder TTS uses dedicated BMO director`

- [ ] 8. English-only INSTRUCTION + fake agent + dev parity

  **What to do**:
  - Rewrite `INSTRUCTION` in `app/agent/adk_agent.py`: BMO understands ID+EN input but MUST respond strictly in English (one concise sentence, device-friendly).
  - Update `app/agent/fake.py` keyword routing to match BOTH Indonesian and English (`catat`/`note`/`task` → create_task, `ingatkan`/`remind` → set_reminder, `pengeluaran`/`expense` → create_expense, `ringkasan`/`summary` → get_today_summary). Fake replies in English.
  - Mirror INSTRUCTION + any signature in `agents/taskbot_agent/agent.py` to keep parity (Hard Rule 7).

  **Must NOT do**: Add `output_schema`. Change tool names/order/signatures. Touch STT.

  **Recommended Agent Profile**: Category `deep`; Skills `[]`.

  **Parallelization**: Wave 3; Blocks T9; Blocked By none.

  **References**:
  - `app/agent/adk_agent.py` — `INSTRUCTION` constant.
  - `app/agent/fake.py` — keyword routing map.
  - `agents/taskbot_agent/agent.py` — dev shell, parity target.
  - `app/tests/test_dev_agent_parity.py` — parity contract.

  **Acceptance Criteria**:
  - [ ] `test_dev_agent_parity.py` passes.
  - [ ] Fake agent routes both `"catat tugas..."` and `"note a task..."` to create_task, replies in English.

  **QA Scenarios**:
  ```
  Scenario: bilingual input, English output
    Tool: Bash
    Steps: 1. AGENT_MODE=fake; run "catat tugas matematika" 2. run "note math task"
           3. assert both → task action, reply is English
    Expected: both routed, English reply
    Evidence: .sisyphus/evidence/task-8-bilingual.txt
  ```

  **Commit**: YES — `feat: English-only replies, bilingual input routing`

- [ ] 9. Update Indonesian-assertion tests to English

  **What to do**:
  - Find tests asserting Indonesian reply text or Indonesian-only keyword routing and update expectations to English (matching the new INSTRUCTION + fake agent).
  - Keep tool-call assertions (action types, ids) unchanged — only the language of expected replies changes.
  - Do NOT delete tests to make them pass; fix expectations.

  **Must NOT do**: Remove/skip failing tests. Weaken assertions beyond the language change.

  **Recommended Agent Profile**: Category `unspecified-high`; Skills `[]`.

  **Parallelization**: Wave 4; Blocks F1; Blocked By T6, T7, T8.

  **References**:
  - `app/tests/` — grep for Indonesian strings (`catat`, `tugas`, `dicatat`, `Pengingat`, `Maaf`) to locate assertions.
  - `app/agent/fake.py` — new English reply strings to match.

  **Acceptance Criteria**:
  - [ ] `python -m pytest -q` reports all green (target ≥319, adjusted for any new tests).
  - [ ] No test deleted or `@skip` added (diff review).

  **QA Scenarios**:
  ```
  Scenario: full suite green
    Tool: Bash
    Steps: 1. python -m pytest -q
    Expected: all pass, 0 failures
    Evidence: .sisyphus/evidence/task-9-pytest.txt
  ```

  **Commit**: YES — `test: update reply assertions to English`

- [ ] 10. Fix `generate_firmware_sounds.py` (path + English + mimo)

  **What to do**:
  - Fix output path bug: `_resolve_output_dir` currently returns `firmware/sd_template/sounds` → must be `firmware/Lyla-Taskbot/sd_template/sounds`.
  - Switch `PHRASES` values to English BMO lines (keep the 10 filenames identical).
  - Add a provider switch (`--provider mimo|gemini`, default `mimo`) so synthesis uses MiMo with an appropriate per-asset director/tag (reuse DIRECTOR_MAP nuance, e.g. `err_generic`→apologetic, `greet_hello`→cheerful).
  - Keep `--force`/`--only`/retry behavior.

  **Must NOT do**: Rename the 10 files. Change WAV format expectations (stay 24kHz mono).

  **Recommended Agent Profile**: Category `unspecified-high`; Skills `[]`.

  **Parallelization**: Wave 4; Blocks T11; Blocked By T4, T5.

  **References**:
  - `scripts/generate_firmware_sounds.py:45-47` — path bug; `:31-42` — PHRASES dict.
  - `app/audio/director_map.py` — per-asset director source.

  **Acceptance Criteria**:
  - [ ] `_resolve_output_dir()` returns a path containing `Lyla-Taskbot/sd_template/sounds`.
  - [ ] `--provider mimo` invokes MimoTtsProvider; PHRASES are English.

  **QA Scenarios**:
  ```
  Scenario: correct path + english phrases
    Tool: Bash
    Steps: 1. python -c print(_resolve_output_dir()) 2. inspect PHRASES values
    Expected: path has Lyla-Taskbot; phrases English
    Evidence: .sisyphus/evidence/task-10-script.txt
  ```

  **Commit**: YES — `fix: firmware sound generator path + English + MiMo`

- [ ] 11. Regenerate 10 firmware WAVs (English, MiMo)

  **What to do**:
  - Run `python -m scripts.generate_firmware_sounds --provider mimo --force` (with `MIMO_API_KEY` set).
  - Verify all 10 files written to `firmware/Lyla-Taskbot/sd_template/sounds/`, each a valid 24kHz mono RIFF WAV, non-empty.

  **Must NOT do**: Commit the WAVs without confirming valid headers. Leave the old Indonesian WAVs mixed in.

  **Recommended Agent Profile**: Category `unspecified-high`; Skills `[]`.

  **Parallelization**: Wave 4; Blocks F3; Blocked By T10.

  **References**:
  - `firmware/Lyla-Taskbot/sd_template/sounds/` — 10 target files.
  - `firmware/.../audio_playback.cpp` — reads RIFF header at runtime; format must be valid WAV.

  **Acceptance Criteria**:
  - [ ] 10 WAVs exist in the correct dir, each starts with `RIFF`, size > 1KB.
  - [ ] Generator summary reports `failed: 0`.

  **QA Scenarios**:
  ```
  Scenario: all assets regenerated valid
    Tool: Bash
    Steps: 1. run generator --provider mimo --force
           2. for each wav assert first 4 bytes == RIFF and size>1024
    Expected: 10/10 valid, failed:0
    Evidence: .sisyphus/evidence/task-11-assets.txt
  ```

  **Commit**: YES — `assets: regenerate firmware sounds in English (MiMo BMO)`

---

## Final Verification Wave

- [ ] F1. Plan compliance audit (oracle)
- [ ] F2. Code quality + AR7 hermeticity (unspecified-high)
- [ ] F3. Manual QA: mimo synth + reminder TTS fetch (unspecified-high)
- [ ] F4. Scope fidelity — agent runtime/STT untouched (deep)

---

## Commit Strategy
Atomic commits per wave. English scope-prefix style. No secrets staged.

## Success Criteria
- `pytest -q` green; AR7 passes.
- mimo synth returns valid 24kHz RIFF WAV.
- 10 EN WAVs in `firmware/Lyla-Taskbot/sd_template/sounds/`.
- agent runtime + STT diff = zero.
