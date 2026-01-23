import logging
import asyncio
import aio_pika
import json
import os
from google.protobuf.json_format import MessageToDict
from .generated import omni_pb2
from .manager import manager

logger = logging.getLogger("MQConsumer")

async def start_consumer():
    rabbitmq_url = os.getenv("RABBITMQ_URL", "amqp://guest:guest@localhost:5672/")
    
    # 重试连接逻辑
    while True:
        try:
            connection = await aio_pika.connect_robust(rabbitmq_url)
            async with connection:
                channel = await connection.channel()
                
                # 声明 Exchange (必须与 Core Router 一致)
                exchange = await channel.declare_exchange(
                    "omni.topic", aio_pika.ExchangeType.TOPIC
                )
                
                # 声明临时队列
                queue = await channel.declare_queue("omni_backend_q", auto_delete=True)
                
                # 绑定队列 (接收 trade.* 下的所有消息)
                await queue.bind(exchange, routing_key="trade.#")
                
                logger.info("[MQ] Connected to RabbitMQ, waiting for messages...")

                async with queue.iterator() as queue_iter:
                    async for message in queue_iter:
                        async with message.process():
                            try:
                                # 1. 解析 Protobuf
                                event = omni_pb2.EventFrame()
                                event.ParseFromString(message.body)
                                
                                # 2. 转换为 Dict (方便转 JSON)
                                # MessageToDict 会自动处理枚举和嵌套结构
                                data_dict = MessageToDict(event, preserving_proto_field_name=True)
                                
                                # 3. 补充一点元数据
                                # data_dict['processed_at'] = ...
                                
                                # 4. 广播给前端
                                json_str = json.dumps(data_dict)
                                await manager.broadcast(json_str)
                                
                                # logger.debug(f"[MQ] Broadcasted: {event.source_id}")
                                
                            except Exception as e:
                                logger.error(f"[MQ] Parse Error: {e}")
                            
        except Exception as e:
            logger.error(f"[MQ] Connection Error: {e}, retrying in 5s...")
            await asyncio.sleep(5)