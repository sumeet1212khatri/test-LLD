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
    
    try:
        # Capture stderr as well to see C++ crash logs
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return json.loads(result.stdout)
    except subprocess.CalledProcessError as e:
        # This catches segfaults or return code 1
        return {"error": f"C++ Binary Crashed (Code {e.returncode}). Stderr: {e.stderr}"}
    except json.JSONDecodeError:
        # This catches if the binary prints text instead of JSON
        return {"error": f"Invalid JSON output from C++: {result.stdout}"}
    except Exception as e:
        # Generic python errors
        return {"error": f"Server execution error: {str(e)}"}
