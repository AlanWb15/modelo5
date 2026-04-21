#property strict

input bool     InpGlobalHistory  = false;
input datetime InpDateFrom       = D'2024.01.01 00:00';
input datetime InpDateTo         = D'2026.12.31 23:59';
input int      InpExportMs       = 500;                // mínimo 500 ms
input string   InpExportFileName = "mt5_stats.csv";

struct SStats
{
   string scope;
   string fromStr;
   string toStr;

   int    totalTrades;
   int    winners;
   int    losers;

   double grossProfit;
   double grossLoss;
   double netProfit;

   double pipsTotalWin;
   double pipsTotalLoss;

   double winrate;
   double avgWin;
   double avgLoss;
   double expectancy;

   double avgPips;
   double avgPipsWin;
   double avgPipsLoss;

   double maxDrawdown;
   double avgDD;
   double maxDDpct;
   double avgDDpct;

   double balance;
   double totalPct;

   double peakPnL;
   double drawdownSum;
   int    drawdownCount;
};

int g_exportMs;
datetime g_from, g_to;

//--------------------------------------------------
int OnInit()
{
   g_from = InpDateFrom;
   g_to   = InpDateTo;

   g_exportMs = InpExportMs;
   if(g_exportMs < 500)
      g_exportMs = 500;

   EventSetMillisecondTimer(g_exportMs);
   ExportStatsToCsv();

   return(INIT_SUCCEEDED);
}

//--------------------------------------------------
void OnDeinit(const int reason)
{
   EventKillTimer();
}

//--------------------------------------------------
void OnTimer()
{
   ExportStatsToCsv();
}

//--------------------------------------------------
void OnTick()
{
   // opcional; si no quieres que exporte en tick, déjalo vacío
}

//--------------------------------------------------
bool CalculateStats(SStats &st)
{
   if(!HistorySelect(g_from, g_to))
   {
      Print("HistorySelect falló: ", GetLastError());
      return false;
   }

   st.totalTrades   = 0;
   st.winners       = 0;
   st.losers        = 0;
   st.grossProfit   = 0;
   st.grossLoss     = 0;
   st.netProfit     = 0;
   st.pipsTotalWin  = 0;
   st.pipsTotalLoss = 0;
   st.winrate       = 0;
   st.avgWin        = 0;
   st.avgLoss       = 0;
   st.expectancy    = 0;
   st.avgPips       = 0;
   st.avgPipsWin    = 0;
   st.avgPipsLoss   = 0;
   st.maxDrawdown   = 0;
   st.avgDD         = 0;
   st.maxDDpct      = 0;
   st.avgDDpct      = 0;
   st.balance       = 0;
   st.totalPct      = 0;
   st.peakPnL       = 0;
   st.drawdownSum   = 0;
   st.drawdownCount = 0;

   double runningPnL = 0;

   int total = HistoryDealsTotal();

   for(int i = 0; i < total; i++)
   {
      ulong ticket = HistoryDealGetTicket(i);
      if(ticket == 0)
         continue;

      long entry = HistoryDealGetInteger(ticket, DEAL_ENTRY);
      if(entry != DEAL_ENTRY_OUT)
         continue;

      string dealSymbol = HistoryDealGetString(ticket, DEAL_SYMBOL);
      if(!InpGlobalHistory && dealSymbol != _Symbol)
         continue;

      double profit = HistoryDealGetDouble(ticket, DEAL_PROFIT)
                    + HistoryDealGetDouble(ticket, DEAL_SWAP)
                    + HistoryDealGetDouble(ticket, DEAL_COMMISSION);

      double volume   = HistoryDealGetDouble(ticket, DEAL_VOLUME);
      double tickVal  = SymbolInfoDouble(dealSymbol, SYMBOL_TRADE_TICK_VALUE);
      double tickSize = SymbolInfoDouble(dealSymbol, SYMBOL_TRADE_TICK_SIZE);

      double pips = 0;
      if(tickVal > 0 && volume > 0)
         pips = (profit / (volume * tickVal)) * tickSize;

      st.totalTrades++;

      if(profit >= 0)
      {
         st.winners++;
         st.grossProfit  += profit;
         st.pipsTotalWin += pips;
      }
      else
      {
         st.losers++;
         st.grossLoss     += MathAbs(profit);
         st.pipsTotalLoss += MathAbs(pips);
      }

      runningPnL += profit;

      if(runningPnL > st.peakPnL)
         st.peakPnL = runningPnL;

      double drawdown = st.peakPnL - runningPnL;

      if(drawdown > st.maxDrawdown)
         st.maxDrawdown = drawdown;

      if(drawdown > 0)
      {
         st.drawdownSum += drawdown;
         st.drawdownCount++;
      }
   }

   st.netProfit   = st.grossProfit - st.grossLoss;
   st.winrate     = (st.totalTrades > 0) ? (double)st.winners / st.totalTrades * 100.0 : 0;
   st.avgWin      = (st.winners > 0) ? st.grossProfit / st.winners : 0;
   st.avgLoss     = (st.losers  > 0) ? st.grossLoss   / st.losers  : 0;
   st.expectancy  = (st.avgWin * (st.winrate / 100.0)) - (st.avgLoss * (1.0 - st.winrate / 100.0));
   st.avgPips     = (st.totalTrades > 0) ? (st.pipsTotalWin - st.pipsTotalLoss) / st.totalTrades : 0;
   st.avgPipsWin  = (st.winners > 0) ? st.pipsTotalWin  / st.winners : 0;
   st.avgPipsLoss = (st.losers  > 0) ? st.pipsTotalLoss / st.losers  : 0;
   st.avgDD       = (st.drawdownCount > 0) ? st.drawdownSum / st.drawdownCount : 0;
   st.maxDDpct    = (st.peakPnL > 0) ? st.maxDrawdown / st.peakPnL * 100.0 : 0;
   st.avgDDpct    = (st.peakPnL > 0) ? st.avgDD / st.peakPnL * 100.0 : 0;
   st.balance     = AccountInfoDouble(ACCOUNT_BALANCE);
   st.totalPct    = (st.balance > 0) ? st.netProfit / st.balance * 100.0 : 0;

   st.scope   = InpGlobalHistory ? "CUENTA GLOBAL" : _Symbol;
   st.fromStr = TimeToString(g_from, TIME_DATE);
   st.toStr   = TimeToString(g_to, TIME_DATE);

   return true;
}

//--------------------------------------------------
void ExportStatsToCsv()
{
   SStats st;
   if(!CalculateStats(st))
      return;

   int handle = FileOpen(InpExportFileName,
                         FILE_WRITE | FILE_CSV | FILE_ANSI | FILE_COMMON,
                         ',');

   if(handle == INVALID_HANDLE)
   {
      Print("Error al abrir CSV: ", GetLastError());
      return;
   }

   FileWrite(handle,
      "timestamp",
      "scope",
      "fromStr",
      "toStr",
      "totalTrades",
      "winners",
      "losers",
      "grossProfit",
      "grossLoss",
      "netProfit",
      "winrate",
      "avgWin",
      "avgLoss",
      "expectancy",
      "pipsTotalWin",
      "pipsTotalLoss",
      "avgPips",
      "avgPipsWin",
      "avgPipsLoss",
      "maxDrawdown",
      "avgDD",
      "maxDDpct",
      "avgDDpct",
      "balance",
      "totalPct"
   );

   FileWrite(handle,
      TimeToString(TimeCurrent(), TIME_DATE | TIME_SECONDS),
      st.scope,
      st.fromStr,
      st.toStr,
      st.totalTrades,
      st.winners,
      st.losers,
      DoubleToString(st.grossProfit,   2),
      DoubleToString(st.grossLoss,     2),
      DoubleToString(st.netProfit,     2),
      DoubleToString(st.winrate,       2),
      DoubleToString(st.avgWin,        2),
      DoubleToString(st.avgLoss,       2),
      DoubleToString(st.expectancy,    2),
      DoubleToString(st.pipsTotalWin,  2),
      DoubleToString(st.pipsTotalLoss, 2),
      DoubleToString(st.avgPips,       2),
      DoubleToString(st.avgPipsWin,    2),
      DoubleToString(st.avgPipsLoss,   2),
      DoubleToString(st.maxDrawdown,   2),
      DoubleToString(st.avgDD,         2),
      DoubleToString(st.maxDDpct,      2),
      DoubleToString(st.avgDDpct,      2),
      DoubleToString(st.balance,       2),
      DoubleToString(st.totalPct,      2)
   );

   FileClose(handle);
}
