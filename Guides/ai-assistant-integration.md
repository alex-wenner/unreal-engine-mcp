# Talking to the Unreal Editor AI Assistant from an MCP Client

This MCP server exposes three tools that let an AI agent drive *any* Unreal
Editor subsystem — including Epic's **AI Assistant** plugin
(`EditorAIAssistantSubsystem`) and popular community chat/LLM plugins such as
[UE5AgentPython](https://github.com/LucidDelta/UE5AgentPython).

> Why these tools are generic: as of Unreal Engine 5.5, no stable public MCP /
> HTTP API is published for Epic's in-editor AI Assistant. The robust way to
> reach it from an external tool is through the Python Editor Script Plugin,
> which can call any exposed editor subsystem. This MCP server ships a thin
> bridge that forwards Python to the editor and marshals the result back.

## Prerequisites

1. Install the `UnrealMCP` plugin (see the repo README).
2. In **Edit → Plugins**, enable:
   - **Python Editor Script Plugin**
   - Any AI Assistant / LLM plugin you want to talk to (for example Epic's
     AI Assistant, or a community plugin like UE5AgentPython).
3. Restart the editor.

## The three bridge tools

| Tool | Purpose |
|------|---------|
| `execute_console_command(command)` | Run any Unreal console command and capture its output. |
| `execute_editor_python(code)` | Run arbitrary Python in-editor (via the `py` console command). Full access to `unreal.*`. |
| `ask_ai_assistant(message, subsystem?, method?)` | Convenience wrapper that tries known AI-Assistant subsystem/method names and returns the reply. |
| `list_editor_subsystems()` | Discovery helper that lists every loaded `unreal.EditorSubsystem` subclass. |

### 1. Discovering the AI Assistant subsystem

```text
> list_editor_subsystems()
["AssetEditorSubsystem", "EditorActorSubsystem", "EditorAIAssistantSubsystem", ...]
```

### 2. Asking the AI Assistant a question

```text
> ask_ai_assistant(message="How do I make my character jump higher?")
```

The tool tries a short list of well-known subsystem/method names
(`EditorAIAssistantSubsystem.send_chat_message`, `AIAssistantSubsystem.ask`,
`CopilotEditorSubsystem.prompt`, `UE5AgentPythonSubsystem.chat`, …) and returns
the first successful reply.

If you already know the exact subsystem and method, pass them explicitly:

```text
> ask_ai_assistant(
    message="Summarize the selected Blueprint",
    subsystem="EditorAIAssistantSubsystem",
    method="send_chat_message",
  )
```

### 3. Falling back to raw Python

For anything not covered by `ask_ai_assistant`, use `execute_editor_python`:

```text
> execute_editor_python(code='''
import unreal
sub = unreal.get_editor_subsystem(unreal.EditorAIAssistantSubsystem)
print(sub.send_chat_message("Explain the current level"))
''')
```

Or a console command:

```text
> execute_console_command(command="stat fps")
```

## Security note

`execute_editor_python` is a **very powerful escape hatch** — it can modify
assets, run shell commands through Python, and read files on the host machine.
Only enable this MCP server on machines where you trust the connected AI
client. Treat it the same way you would an interactive Python console inside
the editor.

## Troubleshooting

- **"'py' command not recognized"** — The Python Editor Script Plugin is not
  enabled. Enable it under **Edit → Plugins** and restart the editor.
- **`ask_ai_assistant` returns `No AI Assistant subsystem/method responded`** —
  Either no AI Assistant plugin is installed, or the one you have uses
  different names. Run `list_editor_subsystems()` to see what's loaded, then
  inspect it with `execute_editor_python` (e.g. `print(dir(sub))`) and call
  `ask_ai_assistant` with explicit `subsystem` and `method` arguments.
- **Blocked by `Output Log`** — The bridge captures text written via
  `print()` / `unreal.log()`. Subsystems that only emit UI events (not log
  output) may return an empty string; use `execute_editor_python` and build
  the response yourself.
