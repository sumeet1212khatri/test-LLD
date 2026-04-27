"""
HFT Exchange Simulator — FastAPI server
Wraps the C++ hft_engine binary via persistent subprocess.
"""
import asyncio
import json
import os
import subprocess
import threading
import time
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel

# ── Engine process wrapper ────────────────────────────────────────────────────
ENGINE_PATH = Path(__file__).parent / "build" / "hft_engine"

class EngineProcess:
    def __init__(self):
        self.proc = None
        self.lock = threading.Lock()
        self._start()

    def _start(self):
        self.proc = subprocess.Popen(
            [str(ENGINE_PATH)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )

    def send(self, cmd: str, payload: dict = None) -> dict:
        if payload is None:
            payload = {}
        line = f"{cmd} {json.dumps(payload)}\n"
        with self.lock:
            try:
                self.proc.stdin.write(line)
                self.proc.stdin.flush()
                response = self.proc.stdout.readline().strip()
                return json.loads(response)
            except Exception as e:
                # Restart engine if it dies
                try:
                    self.proc.kill()
                except Exception:
                    pass
                self._start()
                raise HTTPException(500, f"Engine error: {e}")

engine = EngineProcess()

# ── FastAPI app ───────────────────────────────────────────────────────────────
app = FastAPI(title="HFT Exchange Simulator", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── Pydantic models ───────────────────────────────────────────────────────────
class OrderRequest(BaseModel):
    symbol: str = "AAPL"
    side: str = "BUY"          # BUY | SELL
    type: str = "LIMIT"        # LIMIT | MARKET | IOC | FOK
    price: float = 0.0
    qty: int = 100
    client_id: str = ""
    market_price: float = 0.0

class CancelRequest(BaseModel):
    symbol: str
    id: int

class FeedStepRequest(BaseModel):
    n: int = 10

class BacktestRequest(BaseModel):
    strategy: str = "MarketMaking"  # MarketMaking | Momentum | MeanReversion
    symbol: str = "AAPL"
    ticks: int = 5000
    start_price: float = 185.0

class RiskLimitsRequest(BaseModel):
    max_order_qty: int = 10000
    max_position: int = 500000
    max_notional_usd: float = 50_000_000.0
    max_orders_per_sec: int = 2000

# ── Routes ────────────────────────────────────────────────────────────────────
@app.get("/api/status")
def status():
    return engine.send("status")

@app.get("/api/symbols")
def symbols():
    return engine.send("symbols")

@app.get("/api/book/{symbol}")
def book(symbol: str, depth: int = 10):
    return engine.send("book", {"symbol": symbol})

@app.get("/api/positions")
def positions():
    return engine.send("positions")

@app.get("/api/trades")
def trades():
    return engine.send("trades")

@app.get("/api/orders")
def orders():
    return engine.send("orders")

@app.post("/api/order")
def submit_order(req: OrderRequest):
    return engine.send("order", req.model_dump())

@app.delete("/api/order")
def cancel_order(req: CancelRequest):
    return engine.send("cancel", req.model_dump())

@app.post("/api/feed/step")
def feed_step(req: FeedStepRequest):
    return engine.send("feed_step", {"n": req.n})

@app.post("/api/backtest")
def backtest(req: BacktestRequest):
    result = engine.send("backtest", req.model_dump())
    return result

@app.get("/api/risk/limits")
def get_limits():
    return engine.send("get_limits")

@app.post("/api/risk/limits")
def set_limits(req: RiskLimitsRequest):
    return engine.send("risk_limits", req.model_dump())

@app.post("/api/reset")
def reset():
    return engine.send("reset")

# ── Serve frontend ────────────────────────────────────────────────────────────
@app.get("/")
def serve_index():
    return FileResponse(str(Path(__file__).parent / "index.html"))

if __name__ == "__main__":
    import uvicorn
    port = int(os.environ.get("PORT", 7860))
    uvicorn.run(app, host="0.0.0.0", port=port, log_level="info")