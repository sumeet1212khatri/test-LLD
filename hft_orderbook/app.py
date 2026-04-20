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

    stdout_data = ""
    try:
        # BUG FIX #6: Added timeout=120 — 20M benchmark takes ~35s on cloud VM.
        # Without timeout, HF Space HTTP request silently kills the process at
        # ~30s leaving the frontend hanging with no error message.
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        stdout_data = proc.stdout

        if proc.returncode != 0:
            return {
                "error": f"C++ Binary Crashed (Code {proc.returncode}). Stderr: {proc.stderr}"
            }

        return json.loads(stdout_data)

    except subprocess.TimeoutExpired:
        # BUG FIX #6: Catch timeout explicitly and return clean error JSON
        # instead of crashing the FastAPI worker process.
        return {"error": "Benchmark timed out after 120 seconds. Try fewer orders (1M or 5M)."}

    except json.JSONDecodeError:
        return {"error": f"Invalid JSON output from C++: {stdout_data!r}"}

    except Exception as e:
        return {"error": f"Server execution error: {str(e)}"}
