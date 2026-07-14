import base64
import os
import re
import subprocess
from functools import wraps
from pathlib import Path, PurePosixPath
from urllib.parse import quote

from flask import Flask, Response, jsonify, request, send_file
import requests

app = Flask(__name__)

CHAT_COMPLETIONS_URL = os.environ.get(
    "CHAT_COMPLETIONS_URL",
    "https://api.groq.com/openai/v1/chat/completions",
)
MODEL = os.environ.get("MODEL", os.environ.get("GROQ_MODEL", "llama-3.3-70b-versatile"))
PROVIDER_API_KEY = os.environ.get(
    "PROVIDER_API_KEY",
    os.environ.get("GROQ_API_KEY", os.environ.get("OPENAI_API_KEY", "")),
)
BRIDGE_USER = os.environ.get("BRIDGE_USER", "macuser")
BRIDGE_PASS = os.environ.get("BRIDGE_PASS", "classic922")
WORKSPACE = Path(os.environ.get("WORKSPACE", "/app/workspace")).resolve()
REQUEST_TIMEOUT = int(os.environ.get("REQUEST_TIMEOUT", "90"))
MAX_COMPLETION_TOKENS = int(os.environ.get("MAX_COMPLETION_TOKENS", "1024"))
BUILD_COMMAND = os.environ.get("BUILD_COMMAND", "").strip()
BUILD_TIMEOUT = int(os.environ.get("BUILD_TIMEOUT", "120"))
BUILD_OUTPUT = os.environ.get("BUILD_OUTPUT", "").strip()
RETRO68_ROOT = Path(os.environ.get("RETRO68_ROOT", "/opt/retro68/toolchain")).resolve()
BUILD_PROJECT_DIR = os.environ.get("BUILD_PROJECT_DIR", "").strip()
OS9_DOCS_ROOT = Path(os.environ.get("OS9_DOCS_ROOT", "/app/os9docs-master")).resolve()
MAX_DOC_SNIPPETS = int(os.environ.get("MAX_DOC_SNIPPETS", "4"))
CUSTOM_SYSTEM_INSTRUCTION = os.environ.get("SYSTEM_INSTRUCTION", "").strip()
BASE_SYSTEM_INSTRUCTION = (
    "Answer plainly in short paragraphs for a Mac OS 9 plain text chat client. "
    "Do not use Markdown tables, ASCII art, emojis, or roleplay unless the user asks. "
    "Keep responses concise and readable in a small Classic Mac text window. "
    "When the prompt includes AltanAI Project Context, follow its mode and tool protocol exactly. "
    "In Build mode, if and only if the user explicitly requests code changes, edits, creation, or deletion, "
    "prepare file changes by appending hidden Classic-client tool blocks after a short visible explanation; "
    "the client previews and stores them as pending changes until the user applies them. "
    "If the user is only asking a question or seeking information/explanations, do NOT append any tool blocks (like ALTANAI_PATCH or ALTANAI_WRITE) "
    "and do NOT propose changes; simply answer the question in plain text. "
    "CRITICAL: Always use ALTANAI_PATCH for modifying any existing files. You must NEVER use ALTANAI_WRITE on an existing file, as writing full files overflows the client's 32KB memory limit. ALTANAI_WRITE is strictly reserved for creating new files that do not exist yet. "
    "To modify an existing file, use: "
    "<ALTANAI_PATCH path=\"relative/file.txt\"><ALTANAI_OLD>exact current text</ALTANAI_OLD><ALTANAI_NEW>replacement text</ALTANAI_NEW></ALTANAI_PATCH>. "
    "Use <ALTANAI_APPEND path=\"relative/file.txt\">text</ALTANAI_APPEND> for append requests, "
    "and for repeated text/line appends use <ALTANAI_APPEND path=\"relative/file.txt\" repeat=\"100\">one line</ALTANAI_APPEND> instead of pasting many lines. "
    "<ALTANAI_WRITE path=\"relative/file.txt\">full file content</ALTANAI_WRITE> only for new files, "
    "<ALTANAI_CREATE path=\"relative/file.txt\"/> for empty files, "
    "<ALTANAI_READ path=\"relative/file.txt\"/> to acknowledge reads, and "
    "<ALTANAI_DELETE path=\"relative/file.txt\"/> only for explicit deletion. "
    "Do not use <PATCH>, <REPLACE>, filename=, offset=, or length= syntax; the client only recognizes ALTANAI_* blocks. "
    "Never use absolute paths or .. path segments. "
    "Never claim files were updated, saved, or applied when returning edit tool blocks; say prepared changes instead. "
    "In Plan mode, hidden edit tool blocks are previews only; never claim changes were applied. "
    "For Classic resource SIZE templates, avoid unsupported dontSaveScreen and enableOptionSwitch flags; use reserved in those positions. "
    "For build results, use <ALTANAI_BUILD_RESULT>status: ok/failed; warnings: N; errors: N; output: filename or none</ALTANAI_BUILD_RESULT>. "
    "If you need file contents that were not provided, say so instead of guessing."
)
SYSTEM_INSTRUCTION = (
    BASE_SYSTEM_INSTRUCTION + " " + CUSTOM_SYSTEM_INSTRUCTION
    if CUSTOM_SYSTEM_INSTRUCTION
    else BASE_SYSTEM_INSTRUCTION
)


def wants_json() -> bool:
    return "application/json" in request.headers.get("Accept", "")


def error_response(message: str, status: int):
    if wants_json():
        return jsonify({"ok": False, "error": message, "response": message}), status
    headers = {}
    if status == 401:
        headers["WWW-Authenticate"] = 'Basic realm="AltanAI Bridge"'
    return Response(message + "\n", status=status, mimetype="text/plain", headers=headers)


def success_response(text: str):
    if wants_json():
        return jsonify({"ok": True, "response": text})
    return Response(text, mimetype="text/plain")


def check_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth_header = request.headers.get("Authorization", "")
        if not auth_header.startswith("Basic "):
            return error_response("Missing Basic auth credentials.", 401)

        try:
            decoded = base64.b64decode(auth_header[6:]).decode("utf-8")
            user, password = decoded.split(":", 1)
        except Exception:
            return error_response("Invalid Basic auth header.", 401)

        if user != BRIDGE_USER or password != BRIDGE_PASS:
            return error_response("Invalid bridge username or password.", 401)

        return f(*args, **kwargs)

    return decorated


def workspace_path(filepath: str) -> Path:
    candidate = (WORKSPACE / filepath.lstrip("/")).resolve()
    if candidate != WORKSPACE and WORKSPACE not in candidate.parents:
        raise ValueError("Path traversal blocked.")
    return candidate


def canonical_snapshot_path(filepath: str) -> str:
    raw = (filepath or "").strip().replace("\\", "/")
    parts = PurePosixPath(raw).parts
    if not parts or raw.startswith("/"):
        raise ValueError("Invalid snapshot path.")
    if any(part in ("", ".", "..") for part in parts):
        raise ValueError("Invalid snapshot path.")

    fixed = list(parts)
    if fixed[-1].lower() == "cmakelists.txt":
        fixed[-1] = "CMakeLists.txt"
    return "/".join(fixed)


def import_build_snapshot(raw_body: bytes) -> str:
    text = raw_body.decode("utf-8", errors="replace")
    if "<<<ALTANAI_BUILD_SNAPSHOT>>>" not in text:
        return ""

    imported: list[str] = []
    pattern = re.compile(
        r'<<<ALTANAI_FILE path="([^"]+)">>>\r?\n(.*?)\r?\n<<<ALTANAI_END_FILE>>>',
        re.S,
    )
    for match in pattern.finditer(text):
        rel_path = canonical_snapshot_path(match.group(1))
        content = match.group(2)
        full = workspace_path(rel_path)
        full.parent.mkdir(parents=True, exist_ok=True)
        full.write_text(content, encoding="utf-8", newline="\n")
        imported.append(rel_path)

    if not imported:
        return "Build snapshot received, but no files were imported."
    return "Imported build snapshot: " + ", ".join(imported[:12])


def read_file(filepath: str) -> str:
    try:
        full = workspace_path(filepath)
        return full.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return f"Error: file not found: {filepath}"
    except Exception as exc:
        return f"Error reading file: {exc}"


def write_file(filepath: str, content: str) -> str:
    try:
        full = workspace_path(filepath)
        full.parent.mkdir(parents=True, exist_ok=True)
        full.write_text(content, encoding="utf-8")
        return f"Wrote {len(content)} bytes to {filepath}"
    except Exception as exc:
        return f"Error writing file: {exc}"


def list_files() -> str:
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    files = sorted(path.relative_to(WORKSPACE).as_posix() for path in WORKSPACE.rglob("*") if path.is_file())
    return "\n".join(files) if files else "No files in workspace."


def retro68_available() -> bool:
    return (RETRO68_ROOT / "bin" / "powerpc-apple-macos-gcc").is_file()


def has_cmakelists(path: Path) -> bool:
    if (path / "CMakeLists.txt").is_file():
        return True
    if not path.is_dir():
        return False
    return any(child.is_file() and child.name.lower() == "cmakelists.txt" for child in path.iterdir())


def find_build_project_dir() -> str:
    candidates: list[Path] = []

    if BUILD_PROJECT_DIR:
        project = Path(BUILD_PROJECT_DIR)
        if project.is_absolute():
            candidates.append(project)
        else:
            candidates.append(WORKSPACE / project)

    candidates.extend([WORKSPACE, WORKSPACE / "macos9_client"])

    for candidate in candidates:
        if has_cmakelists(candidate):
            try:
                return candidate.relative_to(WORKSPACE).as_posix() or "."
            except ValueError:
                return str(candidate)

    try:
        found = next(path for path in WORKSPACE.rglob("*")
                     if path.is_file() and path.name.lower() == "cmakelists.txt")
    except StopIteration:
        return ""
    return found.parent.relative_to(WORKSPACE).as_posix() or "."


def docs_available() -> bool:
    return (OS9_DOCS_ROOT / "headers").is_dir()


def infer_doc_headers(prompt: str) -> list[str]:
    lower = prompt.lower()
    selected: list[str] = []
    rules = [
        (("window", "windows", "grafport"), ["MacWindows.h", "Windows.h"]),
        (("dialog", "modal", "alert"), ["Dialogs.h"]),
        (("button", "control", "checkbox", "scrollbar", "slider"), ["Controls.h", "ControlDefinitions.h"]),
        (("menu", "menubar", "menu bar"), ["Menus.h"]),
        (("file", "folder", "fsspec", "hfs", "read", "write"), ["Files.h", "Folders.h"]),
        (("resource", "rsrc", "icon", "icns", "pict"), ["Resources.h", "Icons.h", "PictUtils.h"]),
        (("quickdraw", "draw", "rect", "color", "rgb"), ["Quickdraw.h", "QuickdrawText.h"]),
        (("textedit", "text edit", "tehandle", "teset", "tecaltext"), ["TextEdit.h", "MacTextEditor.h"]),
        (("event", "mouse", "keyboard", "waitnextevent"), ["Events.h"]),
        (("memory", "handle", "ptr", "newhandle", "dispose"), ["Memory.h", "MacMemory.h"]),
        (("open transport", "network", "tcp", "otsnd", "otrcv"), ["OpenTransport.h", "OpenTptInternet.h"]),
        (("process", "launch", "application"), ["Processes.h", "SegLoad.h"]),
    ]

    for needles, headers in rules:
        if any(needle in lower for needle in needles):
            for header in headers:
                if header not in selected:
                    selected.append(header)

    for header_path in sorted((OS9_DOCS_ROOT / "headers").glob("*.h")) if docs_available() else []:
        stem = header_path.stem.lower()
        if stem in lower and header_path.name not in selected:
            selected.append(header_path.name)
        if len(selected) >= MAX_DOC_SNIPPETS:
            break

    return selected[:MAX_DOC_SNIPPETS]


def summarize_doc_header(header_name: str, prompt: str) -> str:
    path = OS9_DOCS_ROOT / "headers" / header_name
    if not path.exists() or not path.is_file():
        return ""

    lower_prompt = prompt.lower()
    terms = {
        word.strip(".,:;()[]{}<>\"'")
        for word in lower_prompt.replace("_", " ").split()
        if len(word.strip(".,:;()[]{}<>\"'")) >= 4
    }
    terms.update(header_name.lower().replace(".h", "").split())

    try:
        lines = path.read_text(encoding="latin-1", errors="replace").splitlines()
    except Exception:
        return ""

    matches: list[str] = []
    for line in lines:
        stripped = line.strip()
        if not stripped:
            continue
        lower_line = stripped.lower()
        if any(term in lower_line for term in terms) or stripped.startswith(("typedef", "EXTERN_API", "#define")):
            matches.append(stripped[:180])
        if len(matches) >= 24:
            break

    if not matches:
        matches = [line.strip()[:180] for line in lines[:24] if line.strip()]

    body = "\n".join(matches)
    if len(body) > 1400:
        body = body[:1400] + "\n[snippet truncated]"
    return f"--- Mac OS 9 Header Snippet: {header_name} ---\n{body}"


def augment_prompt_with_docs(prompt: str) -> str:
    if not docs_available():
        return prompt
    headers = infer_doc_headers(prompt)
    if not headers:
        return prompt

    snippets = [summarize_doc_header(header, prompt) for header in headers]
    snippets = [snippet for snippet in snippets if snippet]
    if not snippets:
        return prompt

    return (
        "--- Local Mac OS 9 API Documentation Snippets ---\n"
        + "\n".join(snippets)
        + "\n--- End Local Mac OS 9 API Documentation Snippets ---\n"
        + prompt
    )


def count_build_diagnostics(text: str) -> tuple[int, int]:
    warnings = 0
    errors = 0
    for line in text.splitlines():
        lower = line.lower()
        if re.search(r"\bwarning\b", lower):
            warnings += 1
        if re.search(r"\berror\b|\bfatal\b|undefined reference|failed", lower):
            errors += 1
    return warnings, errors


def build_artifact_candidates() -> list[Path]:
    project_rel = find_build_project_dir()
    project_root = workspace_path(project_rel) if project_rel and project_rel != "." else WORKSPACE
    if not project_root.exists():
        project_root = WORKSPACE

    suffix_priority = [
        ".dsk",
        ".bin",
        ".APPL",
        ".ad",
        ".pef",
        ".xcoff",
        ".app",
        ".exe",
    ]
    candidates: list[tuple[int, float, Path]] = []
    for path in project_root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(project_root).as_posix()
        if "/CMakeFiles/" in "/" + rel or "/.rsrc/" in "/" + rel or "/.finf/" in "/" + rel:
            continue
        if path.name.startswith("CMakeDetermineCompilerABI"):
            continue
        for priority, suffix in enumerate(suffix_priority):
            if path.name.endswith(suffix):
                candidates.append((priority, -path.stat().st_mtime, path))
                break

    candidates.sort()
    return [path for _, _, path in candidates]


def detect_output_file() -> str:
    if BUILD_OUTPUT:
        output_path = workspace_path(BUILD_OUTPUT)
        if output_path.exists():
            return BUILD_OUTPUT
        return f"{BUILD_OUTPUT} (not found)"

    candidates = build_artifact_candidates()
    if not candidates:
        return "none"
    return candidates[0].relative_to(WORKSPACE).as_posix()


def artifact_download_url(path: str) -> str:
    return f"/api/artifact?path={quote(path, safe='/')}"


def format_build_result(status: str, warnings: int, errors: int, output: str, log: str) -> str:
    max_log = 4000
    log = log.strip()
    if len(log) > max_log:
        log = log[-max_log:]
        log = "[output truncated]\n" + log

    return (
        "<ALTANAI_BUILD_RESULT>"
        f"status: {status}; warnings: {warnings}; errors: {errors}; output: {output}\n"
        f"{log}"
        "</ALTANAI_BUILD_RESULT>"
    )


def append_no_artifact_hint(log: str, output: str) -> str:
    if output != "none":
        return log
    return (
        log
        + "\nNo downloadable Mac OS artifact was found. "
        + "For a user-deliverable app, the project CMakeLists.txt should use "
        + "Retro68 add_application(... TYPE APPL ...), which creates .dsk/.bin/.APPL outputs."
    )


def extract_prompt() -> str:
    if request.is_json:
        data = request.get_json(silent=True) or {}
        prompt = data.get("prompt", "")
    else:
        prompt = request.get_data(as_text=True)

    return prompt.strip()


def extract_project_name() -> str:
    if request.is_json:
        data = request.get_json(silent=True) or {}
        name = data.get("name") or data.get("project") or data.get("prompt") or ""
    else:
        name = request.get_data(as_text=True)
    return (name or "NewMacApp").strip()


chat_history: list[dict[str, str]] = []


def clean_old_user_message(content: str) -> str:
    # Remove file contents
    content = re.sub(
        r'--- Local File Content:.*?\n--- End Local File Content ---\n?',
        '',
        content,
        flags=re.S
    )
    # Remove API docs
    content = re.sub(
        r'--- Local Mac OS 9 API Documentation Snippets ---.*?--- End Local Mac OS 9 API Documentation Snippets ---\n?',
        '',
        content,
        flags=re.S
    )
    # Remove local file operations
    content = re.sub(
        r'--- Local File Operations ---.*?(?=\n--- User Prompt ---|\n--- Local File Content:|\Z)',
        '',
        content,
        flags=re.S
    )
    return content.strip()


def call_model(prompt: str) -> str:
    global chat_history
    # Clean previous user messages in the history to save tokens and avoid duplicate file states
    for msg in chat_history:
        if msg["role"] == "user":
            msg["content"] = clean_old_user_message(msg["content"])

    prompt = augment_prompt_with_docs(prompt)
    print(f"=== PROMPT ===\n{prompt}\n==============", flush=True)
    chat_history.append({"role": "user", "content": prompt})
    if len(chat_history) > 10:
        chat_history = chat_history[-10:]

    payload = {
        "model": MODEL,
        "messages": [
            {"role": "system", "content": SYSTEM_INSTRUCTION},
        ] + chat_history,
        "max_completion_tokens": MAX_COMPLETION_TOKENS,
        "temperature": 0.7,
    }
    headers = {
        "Authorization": f"Bearer {PROVIDER_API_KEY}",
        "Content-Type": "application/json",
        "Accept": "application/json",
    }

    response = requests.post(
        CHAT_COMPLETIONS_URL,
        headers=headers,
        json=payload,
        timeout=REQUEST_TIMEOUT,
    )
    if response.status_code >= 300:
        if chat_history and chat_history[-1]["role"] == "user":
            chat_history.pop()
        try:
            detail = response.json()
        except Exception:
            detail = response.text
        raise RuntimeError(f"provider HTTP {response.status_code}: {detail}")

    data = response.json()
    choices = data.get("choices") or []
    if not choices:
        return ""

    message = choices[0].get("message") or {}
    content = message.get("content") or ""
    print(f"=== RESPONSE ===\n{content}\n================", flush=True)
    if content:
        chat_history.append({"role": "assistant", "content": content})
    return content


@app.route("/api/chat", methods=["POST"])
@check_auth
def chat():
    if not PROVIDER_API_KEY:
        return error_response("Provider API key is not configured on the bridge.", 503)

    prompt = extract_prompt()
    if not prompt:
        return error_response("Empty prompt.", 400)

    try:
        text = call_model(prompt)
    except Exception as exc:
        return error_response(f"Bridge error: {exc}", 502)

    return success_response(text or "No response from provider.")


@app.route("/api/clear", methods=["POST"])
@check_auth
def clear():
    global chat_history
    chat_history = []
    return success_response("Memory cleared.")


@app.route("/api/build", methods=["POST"])
@check_auth
def build():
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    snapshot_note = ""

    try:
        snapshot_note = import_build_snapshot(request.get_data() or b"")
    except Exception as exc:
        return success_response(
            format_build_result("failed", 0, 1, "none", f"Build snapshot import error: {exc}")
        )

    if not BUILD_COMMAND:
        return success_response(
            format_build_result(
                "not configured",
                0,
                0,
                "none",
                (snapshot_note + "\n" if snapshot_note else "")
                + "Set BUILD_COMMAND in the bridge environment to enable the Build button.",
            )
        )

    try:
        completed = subprocess.run(
            BUILD_COMMAND,
            cwd=WORKSPACE,
            shell=True,
            text=True,
            capture_output=True,
            timeout=BUILD_TIMEOUT,
        )
        combined = "\n".join(part for part in [completed.stdout, completed.stderr] if part)
        warnings, errors = count_build_diagnostics(combined)
        if completed.returncode != 0 and errors == 0:
            errors = 1
        status = "ok" if completed.returncode == 0 else "failed"
        output = detect_output_file() if completed.returncode == 0 else "none"
        log = f"{snapshot_note}\n$ {BUILD_COMMAND}\nexit: {completed.returncode}\n{combined}"
        if completed.returncode == 0:
            log = append_no_artifact_hint(log, output)
        return success_response(format_build_result(status, warnings, errors, output, log))
    except subprocess.TimeoutExpired as exc:
        combined = "\n".join(part for part in [exc.stdout or "", exc.stderr or ""] if part)
        warnings, errors = count_build_diagnostics(combined)
        return success_response(
            format_build_result(
                "failed",
                warnings,
                max(errors, 1),
                "none",
                f"$ {BUILD_COMMAND}\ntimeout after {BUILD_TIMEOUT}s\n{combined}",
            )
        )
    except Exception as exc:
        return success_response(
            format_build_result("failed", 0, 1, "none", f"Build bridge error: {exc}")
        )


@app.route("/api/artifacts", methods=["GET"])
@check_auth
def artifacts():
    items = []
    for path in build_artifact_candidates():
        rel = path.relative_to(WORKSPACE).as_posix()
        items.append({
            "path": rel,
            "bytes": path.stat().st_size,
            "url": artifact_download_url(rel),
        })

    if wants_json():
        return jsonify({"ok": True, "artifacts": items})

    if not items:
        return Response("No build artifacts found.\n", mimetype="text/plain")

    lines = [
        f"{item['path']}\t{item['bytes']} bytes\t{item['url']}"
        for item in items
    ]
    return Response("\n".join(lines) + "\n", mimetype="text/plain")


@app.route("/api/artifact", methods=["GET"])
@check_auth
def artifact():
    requested = request.args.get("path", "")
    try:
        rel = canonical_snapshot_path(requested)
        full = workspace_path(rel)
    except ValueError:
        return error_response("Invalid artifact path.", 400)

    allowed = {
        path.relative_to(WORKSPACE).as_posix()
        for path in build_artifact_candidates()
    }
    if rel not in allowed or not full.is_file():
        return error_response("Artifact not found.", 404)

    return send_file(
        full,
        mimetype="application/octet-stream",
        as_attachment=True,
        download_name=full.name,
    )


@app.route("/api/scaffold", methods=["POST"])
@check_auth
def scaffold():
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    name = extract_project_name()

    try:
        completed = subprocess.run(
            ["python", "/app/scaffold_macos9_project.py", str(WORKSPACE), name],
            text=True,
            capture_output=True,
            timeout=20,
        )
    except Exception as exc:
        return error_response(f"Scaffold error: {exc}", 500)

    output = "\n".join(part for part in [completed.stdout, completed.stderr] if part).strip()
    if completed.returncode != 0:
        return error_response(output or "Scaffold failed.", 400)
    return success_response(output or "Project scaffold created.")


@app.route("/health", methods=["GET"])
def health():
    return jsonify(
        {
            "ok": True,
            "provider": "openai-compatible",
            "chat_completions_url": CHAT_COMPLETIONS_URL,
            "model": MODEL,
            "workspace": str(WORKSPACE),
            "api_key_configured": bool(PROVIDER_API_KEY),
            "build_configured": bool(BUILD_COMMAND),
            "build_command": BUILD_COMMAND,
            "build_project_dir": find_build_project_dir(),
            "retro68_configured": retro68_available(),
            "retro68_root": str(RETRO68_ROOT),
            "docs_configured": docs_available(),
            "os9_docs_root": str(OS9_DOCS_ROOT),
        }
    )


if __name__ == "__main__":
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    app.run(host="0.0.0.0", port=8080)
