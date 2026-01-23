import asyncio
import logging
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from contextlib import asynccontextmanager
from .consumer import start_consumer
from .manager import manager

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("OmniBackend")

@asynccontextmanager
async def lifespan(app: FastAPI):
    logger.info("Starting Backend API...")
    # 后台启动 MQ 消费者
    task = asyncio.create_task(start_consumer())
    yield
    logger.info("Shutting down...")

app = FastAPI(lifespan=lifespan)

@app.get("/")
async def root():
    return {"message": "OmniQuant Data Service is Running"}

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            # 保持连接活跃，也可以处理前端发来的指令（如订阅特定频道）
            # 目前只需简单 await
            data = await websocket.receive_text()
            # await manager.broadcast(f"Client says: {data}")
    except WebSocketDisconnect:
        manager.disconnect(websocket)
    except Exception as e:
        logger.error(f"WebSocket Error: {e}")