<script setup>
import { ref, onMounted, computed } from 'vue'

// --- 状态变量 ---
const status = ref('Disconnected')
const marketDataMap = ref(new Map())
const positionsMap = ref(new Map())
const trades = ref([]) 
const orders = ref([]) 
const activeTab = ref('orders')

const accountInfo = ref({
  account_id: '-',
  balance: 0,
  available: 0,
  frozen: 0,
})

// --- 计算属性 ---
const sortedData = computed(() => {
  return Array.from(marketDataMap.value.values()).sort((a, b) => a.tick.symbol.localeCompare(b.tick.symbol))
})

const sortedPositions = computed(() => {
  return Array.from(positionsMap.value.values()).sort((a, b) => a.symbol.localeCompare(b.symbol))
})

const recentOrders = computed(() => {
  return [...orders.value].reverse()
})

const recentTrades = computed(() => {
  return [...trades.value].reverse().slice(0, 20)
})

// --- WebSocket 连接 ---
onMounted(() => {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const host = window.location.hostname
  const port = '9999'
  const wsUrl = `${protocol}//${host}:${port}/ws`
  connectWs(wsUrl)
})

function connectWs(url) {
  const ws = new WebSocket(url)
  ws.onopen = () => status.value = 'Connected'
  ws.onclose = () => {
    status.value = 'Disconnected'
    setTimeout(() => connectWs(url), 3000)
  }
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data)
      
      if (data.tick) {
        marketDataMap.value.set(data.tick.symbol, data)
        marketDataMap.value = new Map(marketDataMap.value) 
      } else if (data.account) {
        accountInfo.value = { ...accountInfo.value, ...data.account }
      } else if (data.position) {
        const key = `${data.position.symbol}_${data.position.direction}`
        if (data.position.volume === 0) {
            positionsMap.value.delete(key)
        } else {
            positionsMap.value.set(key, data.position)
        }
        positionsMap.value = new Map(positionsMap.value)
      } else if (data.trade) {
        trades.value.push(data.trade)
      } else if (data.order) {
        const idx = orders.value.findIndex(o => o.order_id === data.order.order_id)
        if (idx !== -1) orders.value[idx] = data.order
        else orders.value.push(data.order)
      }
    } catch (e) { console.error("WS Data Error:", e) }
  }
}

// --- 按钮点击处理 ---
const handleAction = (action) => {
  alert(`功能 [${action}] 正在开发中...`)
}

// --- 格式化工具 ---
const formatMoney = (val) => val ? val.toLocaleString('en-US', {minimumFractionDigits: 2}) : '0.00'
const formatTime = (ns) => ns ? new Date(parseInt(ns)/1000000).toLocaleTimeString('en-GB') : '--:--:--'
const getPnlClass = (val) => val > 0 ? 'text-red' : (val < 0 ? 'text-green' : '')
</script>

<template>
  <div class="app-container">
    <!-- 顶栏与工具栏 -->
    <header class="top-bar">
      <div class="brand">
        <div class="logo-icon">Q</div>
        <h1>OmniQuant</h1>
      </div>

      <div class="toolbar">
        <button class="btn" @click="handleAction('Trade')">⚡ 交易</button>
        <button class="btn" @click="handleAction('Analysis')">📊 分析</button>
        <button class="btn" @click="handleAction('History')">📜 历史</button>
        <button class="btn" @click="handleAction('Settings')">⚙️ 设置</button>
      </div>
      
      <div class="header-right">
        <div class="account-pill" v-if="accountInfo.account_id !== '-'">
          <div class="acc-info">
            <span class="label">权益</span>
            <span class="val">{{ formatMoney(accountInfo.balance) }}</span>
          </div>
          <div class="sep"></div>
          <div class="acc-info">
            <span class="label">可用</span>
            <span class="val">{{ formatMoney(accountInfo.available) }}</span>
          </div>
        </div>
        <div :class="['status-indicator', status.toLowerCase()]">{{ status }}</div>
      </div>
    </header>

    <!-- 主界面布局 -->
    <main class="dashboard-grid">
      
      <!-- 行情面板 -->
      <section class="panel market-panel">
        <div class="panel-header"><h3>行情监控</h3><span class="badge">{{ sortedData.length }}</span></div>
        <div class="table-scroll">
          <table class="data-table">
            <thead>
              <tr><th>合约</th><th class="text-right">最新价</th><th class="text-right">成交量</th><th class="text-right">时间</th></tr>
            </thead>
            <tbody>
              <tr v-for="item in sortedData" :key="item.tick.symbol">
                <td class="symbol">{{ item.tick.symbol }}</td>
                <td class="text-right price">{{ item.tick.last_price }}</td>
                <td class="text-right text-muted">{{ item.tick.volume }}</td>
                <td class="text-right time">{{ formatTime(item.timestamp_ns) }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>

      <!-- 持仓面板 -->
      <section class="panel pos-panel">
        <div class="panel-header"><h3>当前持仓</h3></div>
        <div class="table-scroll">
          <table class="data-table">
            <thead>
              <tr><th>合约</th><th>方向</th><th class="text-right">数量</th><th class="text-right">成本</th><th class="text-right">盈亏</th></tr>
            </thead>
            <tbody>
              <tr v-for="pos in sortedPositions" :key="pos.symbol + pos.direction">
                <td class="symbol">{{ pos.symbol }}</td>
                <td><span :class="['tag', pos.direction.toLowerCase()]">{{ pos.direction === 'Long' ? '多' : '空' }}</span></td>
                <td class="text-right">{{ pos.volume }}</td>
                <td class="text-right">{{ formatMoney(pos.open_price) }}</td>
                <td class="text-right font-bold" :class="getPnlClass(pos.pnl)">{{ formatMoney(pos.pnl) }}</td>
              </tr>
               <tr v-if="sortedPositions.length === 0"><td colspan="5" class="empty-state">暂无持仓</td></tr>
            </tbody>
          </table>
        </div>
      </section>

      <!-- 委托与成交 (Tab) -->
      <section class="panel log-panel">
        <div class="panel-header tab-header">
          <button :class="['tab-btn', activeTab === 'orders' ? 'active' : '']" @click="activeTab = 'orders'">委托</button>
          <button :class="['tab-btn', activeTab === 'trades' ? 'active' : '']" @click="activeTab = 'trades'">成交</button>
        </div>
        
        <div class="list-scroll">
          <div v-if="activeTab === 'orders'">
            <div v-for="order in recentOrders" :key="order.order_id" class="trade-item">
              <div class="trade-info">
                <span class="symbol">{{ order.symbol }}</span>
                <span :class="['dir', order.direction === 'Buy' ? 'text-red' : 'text-green']">
                  {{ order.direction === 'Buy' ? '买' : '卖' }} {{ order.offset }}
                </span>
              </div>
              <div class="trade-detail">
                <div class="price">{{ order.volume }} @ {{ order.price }}</div>
                <div :class="['status-tag', 's'+order.status]">{{ order.status }}</div>
              </div>
            </div>
            <div v-if="recentOrders.length === 0" class="empty-state">暂无委托</div>
          </div>

          <div v-if="activeTab === 'trades'">
            <div v-for="trade in recentTrades" :key="trade.trade_id" class="trade-item">
              <div class="trade-info">
                <span class="symbol">{{ trade.symbol }}</span>
                <span :class="['dir', trade.direction === 'Buy' ? 'text-red' : 'text-green']">
                    {{ trade.direction === 'Buy' ? '买' : '卖' }} {{ trade.offset }}
                </span>
              </div>
              <div class="trade-price">{{ trade.volume }} @ {{ trade.price }}</div>
            </div>
            <div v-if="recentTrades.length === 0" class="empty-state">暂无成交</div>
          </div>
        </div>
      </section>
    </main>
  </div>
</template>

<style>
:root {
  --bg-color: #1e1e1e;
  --card-bg: #252526;
  --text-primary: #e0e0e0;
  --text-secondary: #858585;
  --accent-color: #007acc;
  --border-color: #333333;
  --header-bg: #333333;
  --red: #ff5252;
  --green: #4caf50;
}

body { margin: 0; background-color: var(--bg-color); color: var(--text-primary); font-family: 'Segoe UI', sans-serif; }

.app-container { display: flex; flex-direction: column; height: 100vh; overflow: hidden; }

.top-bar {
  background: var(--header-bg);
  height: 50px;
  display: flex;
  align-items: center;
  padding: 0 16px;
  justify-content: space-between;
  border-bottom: 1px solid #111;
}

.brand { display: flex; align-items: center; gap: 10px; width: 180px; }
.logo-icon { width: 24px; height: 24px; background: var(--accent-color); color: white; border-radius: 4px; display: flex; justify-content: center; align-items: center; font-weight: bold; }
.brand h1 { font-size: 1rem; margin: 0; font-weight: 500; color: #fff; }

.toolbar { display: flex; gap: 4px; }
.btn {
  background: transparent;
  border: none;
  color: var(--text-primary);
  padding: 6px 12px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.85rem;
  transition: background 0.2s;
}
.btn:hover { background: rgba(255,255,255,0.1); }

.header-right { display: flex; align-items: center; gap: 16px; }
.account-pill {
  background: rgba(0,0,0,0.2);
  padding: 4px 12px;
  border-radius: 4px;
  display: flex;
  align-items: center;
  gap: 12px;
}
.acc-info { display: flex; flex-direction: column; line-height: 1.1; }
.acc-info .label { font-size: 0.65rem; color: #aaa; text-transform: uppercase; }
.acc-info .val { font-size: 0.9rem; font-family: 'Consolas', monospace; font-weight: 600; color: #fff; }
.sep { width: 1px; height: 20px; background: #555; }

.status-indicator { font-size: 0.7rem; padding: 2px 8px; border-radius: 10px; font-weight: 600; text-transform: uppercase; }
.status-indicator.connected { background: #1b5e20; color: #81c784; }
.status-indicator.disconnected { background: #b71c1c; color: #ff8a80; }

.dashboard-grid {
  display: grid;
  grid-template-columns: 320px 1fr 320px;
  gap: 2px;
  padding: 2px;
  flex: 1;
  overflow: hidden;
  background: #000;
}

.panel { background: var(--card-bg); display: flex; flex-direction: column; overflow: hidden; }
.panel-header { background: #2d2d2d; border-bottom: 1px solid #111; padding: 8px 12px; display: flex; justify-content: space-between; align-items: center; }
.panel-header h3 { font-size: 0.85rem; color: #ccc; margin: 0; }

.table-scroll { flex: 1; overflow-y: auto; }
.data-table { width: 100%; border-collapse: collapse; }
.data-table th { position: sticky; top: 0; background: #2d2d2d; color: #888; padding: 8px 12px; font-size: 0.75rem; text-align: left; border-bottom: 1px solid #444; }
.data-table td { border-bottom: 1px solid #333; color: #ddd; padding: 8px 12px; font-size: 0.85rem; }
.data-table tr:hover { background: #333; }

.symbol { color: #569cd6; font-weight: 600; }
.price { font-family: 'Consolas', monospace; font-weight: 600; }
.time { font-size: 0.8rem; color: #777; }

.tag { padding: 2px 6px; font-size: 0.75rem; border-radius: 2px; font-weight: 600; }
.tag.long { background: rgba(255, 82, 82, 0.15); color: #ff5252; border: 1px solid #ff5252; }
.tag.short { background: rgba(76, 175, 80, 0.15); color: #4caf50; border: 1px solid #4caf50; }
.text-red { color: #ff5252; }
.text-green { color: #4caf50; }
.font-bold { font-weight: 600; }

.tab-header { padding: 0 !important; display: flex; }
.tab-btn { flex: 1; background: transparent; border: none; color: #888; padding: 10px; cursor: pointer; font-weight: 600; border-bottom: 2px solid transparent; font-size: 0.8rem; }
.tab-btn.active { color: #fff; border-bottom: 2px solid var(--accent-color); background: rgba(255,255,255,0.03); }

.list-scroll { flex: 1; overflow-y: auto; padding: 8px; }
.trade-item { display: flex; justify-content: space-between; padding: 10px; border-bottom: 1px solid #333; }
.trade-info { display: flex; flex-direction: column; gap: 4px; }
.trade-detail { text-align: right; }
.status-tag { font-size: 0.7rem; padding: 2px 4px; border: 1px solid #555; border-radius: 2px; margin-top: 4px; display: inline-block; }

.empty-state { padding: 40px; text-align: center; color: #555; font-size: 0.9rem; }
.badge { background: #444; color: #aaa; font-size: 0.7rem; padding: 2px 6px; border-radius: 10px; }
</style>