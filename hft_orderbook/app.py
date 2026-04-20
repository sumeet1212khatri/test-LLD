from fastapi import FastAPI
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
import subprocess
import json

app = FastAPI()

app.mount("/frontend", StaticFiles(directory="frontend"), name="frontend")

@app.get("/", response_class=HTMLResponse)
async def root():
    with open("frontend/index.html", "r") as f:
        return f.read()

@app.get("/api/benchmark")
async def run_benchmark(orders: int = 1000000, cancel: float = 0.20, market: float = 0.10):
    cmd = [
        "./build/benchmark",
        "--orders", str(orders),
        "--cancel-ratio", str(cancel),
        "--market-ratio", str(market),
        "--json",
        "--quiet"
    ]

    # BUG FIX: Original code had `result` referenced in the JSONDecodeError
    # handler, but `result` is only assigned inside the try block when
    # check=True raises CalledProcessError before the assignment can happen.
    # This would throw a NameError, masking the real error. Fix: capture
    # stdout explicitly before calling check, so result is always defined.
    stdout_data = ""
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True)
        stdout_data = proc.stdout

        if proc.returncode != 0:
            return {
                "error": f"C++ Binary Crashed (Code {proc.returncode}). Stderr: {proc.stderr}"
            }

        return json.loads(stdout_data)

    except json.JSONDecodeError:
        return {"error": f"Invalid JSON output from C++: {stdout_data!r}"}
    except Exception as e:
        return {"error": f"Server execution error: {str(e)}"}
