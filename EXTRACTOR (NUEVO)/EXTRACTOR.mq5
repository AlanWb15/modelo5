#property strict

input bool     InpGlobalHistory   = false;
input datetime InpDateFrom        = D'2024.01.01 00:00';
input datetime InpDateTo          = D'2026.12.31 23:59';
input int      InpExportMs        = 500;                  // mínimo 500 ms
input string   InpBaseFileName    = "mt5_stats";
input bool     InpIncludeLogin    = true;

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
   double equity;
   double freeMargin;
   double margin;
   double marginLevel;
   double totalPct;

   double peakPnL;
   double drawdownSum;
   int    drawdownCount;
};

int      g_exportMs;
datetime g_from, g_to;
string   g_fileName = "";

//--------------------------------------------------
string CleanFilePart(string text)
{
   string badChars[] = {"\\","/",":","*","?","\"","<",">","|"};
   for(int i=0; i<ArraySize(badChars); i++)
      text = StringReplace(text, badChars[i], "_");

   text = StringReplace(text, " ", "_");
   text = StringReplace(text, ".", "_");
   text = StringReplace(text, ",", "_");
   text = StringReplace(text, ";", "_");

   while(StringFind(text, "__") >= 0)
      text = StringReplace(text, "__", "_");

   return text;
}

//--------------------------------------------------
string DealTypeToText(long dealType)
{
   switch(dealType)
   {
      case DEAL_TYPE_BUY:            return "BUY";
      case DEAL_TYPE_SELL:           return "SELL";
      case DEAL_TYPE_BALANCE:        return "BALANCE";
      case DEAL_TYPE_CREDIT:         return "CREDIT";
      case DEAL_TYPE_CHARGE:         return "CHARGE";
      case DEAL_TYPE_CORRECTION:     return "CORRECTION";
      case DEAL_TYPE_BONUS:          return "BONUS";
      case DEAL_TYPE_COMMISSION:     return "COMMISSION";
      case DEAL_TYPE_COMMISSION_DAILY:return "COMM_DAILY";
      case DEAL_TYPE_COMMISSION_MONTHLY:return "COMM_MONTHLY";
      case DEAL_TYPE_COMMISSION_AGENT_DAILY:return "COMM_AGENT_D";
      case DEAL_TYPE_COMMISSION_AGENT_MONTHLY:return "COMM_AGENT_M";
      case DEAL_TYPE_INTEREST:       return "INTEREST";
      case DEAL_TYPE_BUY_CANCELED:   return "BUY_CANCELED";
      case DEAL_TYPE_SELL_CANCELED:  return "SELL_CANCELED";
      default:                       return "OTHER";
   }
}

//--------------------------------------------------
string BuildFileName()
{
   string owner = AccountInfoString(ACCOUNT_NAME);
   owner = CleanFilePart(owner);

   long login = AccountInfoInteger(ACCOUNT_LOGIN);

   string symbolPart = InpGlobalHistory ? "GLOBAL" : _Symbol;
   symbolPart = CleanFilePart(symbolPart);

   string fileName = InpBaseFileName + "_" + owner;

   if(InpIncludeLogin)
      fileName += "_" + IntegerToString((int)login);

   fileName += "_" + symbolPart + ".csv";

   return fileName;
}

//--------------------------------------------------
int OnInit()
{
   g_from = InpDateFrom;
   g_to   = InpDateTo;

   g_exportMs = InpExportMs;
   if(g_exportMs < 500)
      g_exportMs = 500;

   g_fileName = BuildFileName();

   EventSetMillisecondTimer(g_exportMs);
   ExportStatsToCsv();

   Print("Exportando CSV cada ", g_exportMs, " ms en Common\\Files\\", g_fileName);
   Print("Ruta CommonData: ", TerminalInfoString(TERMINAL_COMMONDATA_PATH));

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
   st.equity        = 0;
   st.freeMargin    = 0;
   st.margin        = 0;
   st.marginLevel   = 0;
   st.totalPct      = 0;
   st.peakPnL       = 0;
   st.drawdownSum   = 0;
   st.drawdownCount = 0;

   double runningPnL = 0.0;
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

      double pips = 0.0;
      if(tickVal > 0.0 && volume > 0.0 && tickSize > 0.0)
         pips = (profit / (volume * tickVal)) * tickSize;

      st.totalTrades++;

      if(profit >= 0.0)
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

      if(drawdown > 0.0)
      {
         st.drawdownSum += drawdown;
         st.drawdownCount++;
      }
   }

   st.netProfit   = st.grossProfit - st.grossLoss;
   st.winrate     = (st.totalTrades > 0) ? ((double)st.winners / st.totalTrades) * 100.0 : 0.0;
   st.avgWin      = (st.winners > 0) ? st.grossProfit / st.winners : 0.0;
   st.avgLoss     = (st.losers  > 0) ? st.grossLoss   / st.losers  : 0.0;
   st.expectancy  = (st.avgWin * (st.winrate / 100.0)) - (st.avgLoss * (1.0 - st.winrate / 100.0));
   st.avgPips     = (st.totalTrades > 0) ? (st.pipsTotalWin - st.pipsTotalLoss) / st.totalTrades : 0.0;
   st.avgPipsWin  = (st.winners > 0) ? st.pipsTotalWin  / st.winners : 0.0;
   st.avgPipsLoss = (st.losers  > 0) ? st.pipsTotalLoss / st.losers  : 0.0;
   st.avgDD       = (st.drawdownCount > 0) ? st.drawdownSum / st.drawdownCount : 0.0;
   st.maxDDpct    = (st.peakPnL > 0.0) ? (st.maxDrawdown / st.peakPnL) * 100.0 : 0.0;
   st.avgDDpct    = (st.peakPnL > 0.0) ? (st.avgDD / st.peakPnL) * 100.0 : 0.0;

   st.balance     = AccountInfoDouble(ACCOUNT_BALANCE);
   st.equity      = AccountInfoDouble(ACCOUNT_EQUITY);
   st.freeMargin  = AccountInfoDouble(ACCOUNT_MARGIN_FREE);
   st.margin      = AccountInfoDouble(ACCOUNT_MARGIN);
   st.marginLevel = AccountInfoDouble(ACCOUNT_MARGIN_LEVEL);
   st.totalPct    = (st.balance > 0.0) ? (st.netProfit / st.balance) * 100.0 : 0.0;

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

   int handle = FileOpen(g_fileName,
                         FILE_WRITE | FILE_CSV | FILE_ANSI | FILE_COMMON,
                         ',');

   if(handle == INVALID_HANDLE)
   {
      Print("Error al abrir CSV: ", GetLastError());
      return;
   }

   // ===== RESUMEN =====
   FileWrite(handle, "SECTION", "SUMMARY");

   FileWrite(handle,
      "timestamp",
      "account_name",
      "account_login",
      "server",
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
      "equity",
      "freeMargin",
      "margin",
      "marginLevel",
      "totalPct"
   );

   FileWrite(handle,
      TimeToString(TimeCurrent(), TIME_DATE | TIME_SECONDS),
      AccountInfoString(ACCOUNT_NAME),
      (string)AccountInfoInteger(ACCOUNT_LOGIN),
      AccountInfoString(ACCOUNT_SERVER),
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
      DoubleToString(st.equity,        2),
      DoubleToString(st.freeMargin,    2),
      DoubleToString(st.margin,        2),
      DoubleToString(st.marginLevel,   2),
      DoubleToString(st.totalPct,      2)
   );

   // línea vacía
   FileWrite(handle, "");

   // ===== HISTORIAL =====
   FileWrite(handle, "SECTION", "HISTORY");

   FileWrite(handle,
      "close_time",
      "ticket",
      "position_id",
      "symbol",
      "type",
      "volume",
      "price",
      "profit",
      "swap",
      "commission",
      "net",
      "comment"
   );

   int total = HistoryDealsTotal();

   for(int i = total - 1; i >= 0; i--)
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

      datetime dealTime   = (datetime)HistoryDealGetInteger(ticket, DEAL_TIME);
      long     dealType   = HistoryDealGetInteger(ticket, DEAL_TYPE);
      double   volume     = HistoryDealGetDouble(ticket, DEAL_VOLUME);
      double   price      = HistoryDealGetDouble(ticket, DEAL_PRICE);
      double   profitOnly = HistoryDealGetDouble(ticket, DEAL_PROFIT);
      double   swap       = HistoryDealGetDouble(ticket, DEAL_SWAP);
      double   comm       = HistoryDealGetDouble(ticket, DEAL_COMMISSION);
      double   net        = profitOnly + swap + comm;
      ulong    posId      = (ulong)HistoryDealGetInteger(ticket, DEAL_POSITION_ID);
      string   comment    = HistoryDealGetString(ticket, DEAL_COMMENT);

      FileWrite(handle,
         TimeToString(dealTime, TIME_DATE | TIME_SECONDS),
         (string)ticket,
         (string)posId,
         dealSymbol,
         DealTypeToText(dealType),
         DoubleToString(volume, 2),
         DoubleToString(price, _Digits),
         DoubleToString(profitOnly, 2),
         DoubleToString(swap, 2),
         DoubleToString(comm, 2),
         DoubleToString(net, 2),
         comment
      );
   }

   FileClose(handle);
}
