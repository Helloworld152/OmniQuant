use futures::StreamExt;
use lapin::{options::*, types::FieldTable, BasicProperties, Connection, ConnectionProperties};
use prost::Message;
use std::error::Error;
use zeromq::{Socket, SocketRecv};

// 引入自动生成的 Protobuf 代码
pub mod omni {
    include!(concat!(env!("OUT_DIR"), "/omni.rs"));
}

use std::env;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    env_logger::init();
    println!("[CORE_ROUTER] 正在启动 OmniQuant 核心路由...");

    // 读取配置
    let rabbitmq_addr = env::var("RABBITMQ_URL").unwrap_or_else(|_| "amqp://guest:guest@127.0.0.1:5672/%2f".to_string());
    let zmq_bind_addr = env::var("CORE_ZMQ_BIND").unwrap_or_else(|_| "tcp://127.0.0.1:5555".to_string());

    // 1. 设置 RabbitMQ 连接
    let channel = match Connection::connect(&rabbitmq_addr, ConnectionProperties::default()).await {
        Ok(conn) => {
            let ch = conn.create_channel().await?;
            ch.exchange_declare(
                "omni.topic",
                lapin::ExchangeKind::Topic,
                ExchangeDeclareOptions::default(),
                FieldTable::default(),
            ).await?;
            println!("[CORE_ROUTER] 已成功连接至 RabbitMQ。");
            Some(ch)
        }
        Err(e) => {
            println!("[CORE_ROUTER] 警告：无法连接至 RabbitMQ ({})。消息转发功能将禁用.", e);
            None
        }
    };

    // 2. 设置 ZeroMQ PULL 套接字
    let mut socket = zeromq::PullSocket::new();
    socket.bind(&zmq_bind_addr).await?; 
    
    println!("[CORE_ROUTER] 正在监听 ZMQ {}", zmq_bind_addr);

    // 3. 主循环
    loop {
        match socket.recv().await {
            Ok(msg) => {
                // ZMQ 消息可能包含多个帧，通常第 0 帧为负载数据
                if let Some(payload) = msg.get(0) {
                    // 解析 Protobuf
                    match omni::EventFrame::decode(payload.as_ref()) {
                        Ok(event) => {
                            // 打印业务数据，行情数据 (Tick) 默认静默处理以减少刷屏
                            if let Some(payload_type) = &event.payload {
                                match payload_type {
                                    omni::event_frame::Payload::Tick(_) => {
                                        // 行情数据仅转发，不打印
                                    },
                                    omni::event_frame::Payload::Account(acc) => {
                                        println!("[CORE_ROUTER] [资金] 账户: {} 权益: {} 可用: {}", 
                                            acc.account_id, acc.balance, acc.available);
                                    },
                                    omni::event_frame::Payload::Position(pos) => {
                                        println!("[CORE_ROUTER] [持仓] 合约: {} 方向: {} 数量: {}", 
                                            pos.symbol, pos.direction, pos.volume);
                                    },
                                    omni::event_frame::Payload::Order(ord) => {
                                        println!("[CORE_ROUTER] [委托] 合约: {} 状态: {} 价格: {} 数量: {}", 
                                            ord.symbol, ord.status, ord.price, ord.volume);
                                    },
                                    omni::event_frame::Payload::Trade(trd) => {
                                        println!("[CORE_ROUTER] [成交] 合约: {} 价格: {} 数量: {}", 
                                            trd.symbol, trd.price, trd.volume);
                                    },
                                    _ => println!("[CORE_ROUTER] 收到其他类型的事件: {:?}", event.source_id),
                                }
                            }
                            
                            // 如果 RabbitMQ 可用，则转发消息
                            if let Some(ref ch) = channel {
                                let routing_key = format!("trade.{}", event.source_id);
                                let payload_bytes = event.encode_to_vec();
                                let _ = ch.basic_publish(
                                    "omni.topic",
                                    &routing_key,
                                    BasicPublishOptions::default(),
                                    &payload_bytes,
                                    BasicProperties::default(),
                                ).await;
                            }
                        }
                        Err(e) => eprintln!("[CORE_ROUTER] Protobuf 解析失败: {}", e),
                    }
                }
            }
            Err(e) => eprintln!("[CORE_ROUTER] ZMQ 错误: {}", e),
        }
    }
}