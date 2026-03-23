#property strict

//==================================================
// DEBUG DEFINES — se calculan dinámicamente según escala
//==================================================
#define DBG_X        10
#define DBG_Y        10

//==================================================
// INPUTS
//==================================================
input bool     InpGlobalHistory = false;
input datetime InpDateFrom      = D'2024.01.01 00:00';
input datetime InpDateTo        = D'2025.12.31 23:59';
input int      InpPanelScale    = 1;   // 1 = pequeño | 2 = mediano | 3 = grande

//==================================================
// COLORES
//==================================================
#define CLR_BG        C'20,40,70'
#define CLR_TITLE     C'100,180,255'
#define CLR_SECTION   C'60,120,200'
#define CLR_LABEL     C'200,220,240'
#define CLR_VALUE_POS C'80,220,120'
#define CLR_VALUE_NEG C'220,80,80'
#define CLR_VALUE_NEU C'220,220,180'
#define CLR_SEPARATOR C'50,80,120'

//==================================================
// GLOBALS
//==================================================
string   g_prefix   = "DBGSTAT_";
datetime g_from, g_to;

int g_panelW, g_panelH, g_lineH, g_fontSize, g_lineX, g_lineY;

//==================================================
// ESCALA
//==================================================
void ApplyScale(){
   int scale = InpPanelScale;
   if(scale < 1) scale = 1;
   if(scale > 3) scale = 3;

   switch(scale){
      case 1:
         g_panelW  = 275;
         g_panelH  = 520;
         g_lineH   = 16;
         g_fontSize= 7;
         break;
      case 2:
         g_panelW  = 360;
         g_panelH  = 680;
         g_lineH   = 21;
         g_fontSize= 9;
         break;
      case 3:
         g_panelW  = 460;
         g_panelH  = 860;
         g_lineH   = 27;
         g_fontSize= 12;
         break;
   }
   g_lineX = DBG_X + 12;
   g_lineY = DBG_Y + 12;
}

//==================================================
// INIT / DEINIT
//==================================================
int OnInit(){
   g_from = InpDateFrom;
   g_to   = InpDateTo;
   ApplyScale();
   DrawPanel();
   UpdateStats();
   return INIT_SUCCEEDED;
}

void OnDeinit(const int reason){
   ObjectsDeleteAll(0, g_prefix);
}

void OnTick(){
   UpdateStats();
}

//==================================================
// PANEL BASE
//==================================================
void DrawPanel(){
   string bg = g_prefix+"BG";
   ObjectCreate(0, bg, OBJ_RECTANGLE_LABEL, 0, 0, 0);
   ObjectSetInteger(0, bg, OBJPROP_XDISTANCE,   DBG_X);
   ObjectSetInteger(0, bg, OBJPROP_YDISTANCE,   DBG_Y);
   ObjectSetInteger(0, bg, OBJPROP_XSIZE,       g_panelW);
   ObjectSetInteger(0, bg, OBJPROP_YSIZE,       g_panelH);
   ObjectSetInteger(0, bg, OBJPROP_BGCOLOR,     CLR_BG);
   ObjectSetInteger(0, bg, OBJPROP_BORDER_TYPE, BORDER_FLAT);
   ObjectSetInteger(0, bg, OBJPROP_COLOR,       CLR_SEPARATOR);
   ObjectSetInteger(0, bg, OBJPROP_WIDTH,       1);
   ObjectSetInteger(0, bg, OBJPROP_BACK,        false);
   ObjectSetInteger(0, bg, OBJPROP_SELECTABLE,  false);
}

//==================================================
// HELPERS
//==================================================
void DrawLabel(string name, int line, string txt, color clr){
   string obj = g_prefix + name;
   if(ObjectFind(0, obj) < 0)
      ObjectCreate(0, obj, OBJ_LABEL, 0, 0, 0);
   ObjectSetInteger(0, obj, OBJPROP_XDISTANCE,  g_lineX);
   ObjectSetInteger(0, obj, OBJPROP_YDISTANCE,  g_lineY + line * g_lineH);
   ObjectSetInteger(0, obj, OBJPROP_COLOR,      clr);
   ObjectSetInteger(0, obj, OBJPROP_FONTSIZE,   g_fontSize);
   ObjectSetString (0, obj, OBJPROP_FONT,       "Courier New");
   ObjectSetString (0, obj, OBJPROP_TEXT,       txt);
   ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
   ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
}

void DrawSeparator(string name, int line){
   DrawLabel(name, line, "-------------------------------------------", CLR_SEPARATOR);
}

string Pad(string label, int width = 22){
   int spaces = width - StringLen(label);
   string result = label;
   for(int i = 0; i < spaces; i++) result += " ";
   return result;
}

color ValueColor(double val){
   if(val > 0) return CLR_VALUE_POS;
   if(val < 0) return CLR_VALUE_NEG;
   return CLR_VALUE_NEU;
}

//==================================================
// ESTADISTICAS
//==================================================
void UpdateStats(){
   if(!HistorySelect(g_from, g_to)) return;

   int    totalTrades   = 0;
   int    winners       = 0;
   int    losers        = 0;
   double grossProfit   = 0;
   double grossLoss     = 0;
   double pipsTotalWin  = 0;
   double pipsTotalLoss = 0;
   double runningPnL    = 0;
   double peakPnL       = 0;
   double maxDrawdown   = 0;
   double drawdownSum   = 0;
   int    drawdownCount = 0;
   double consecLoss    = 0;
   double maxConsecLoss = 0;

   int total = HistoryDealsTotal();

   for(int i = 0; i < total; i++){
      ulong ticket = HistoryDealGetTicket(i);
      if(ticket == 0) continue;

      if(HistoryDealGetInteger(ticket, DEAL_ENTRY) != DEAL_ENTRY_OUT) continue;

      string dealSymbol = HistoryDealGetString(ticket, DEAL_SYMBOL);
      if(!InpGlobalHistory && dealSymbol != _Symbol) continue;

      double profit   = HistoryDealGetDouble(ticket, DEAL_PROFIT)
                      + HistoryDealGetDouble(ticket, DEAL_SWAP)
                      + HistoryDealGetDouble(ticket, DEAL_COMMISSION);
      double volume   = HistoryDealGetDouble(ticket, DEAL_VOLUME);
      double tickVal  = SymbolInfoDouble(dealSymbol, SYMBOL_TRADE_TICK_VALUE);
      double tickSize = SymbolInfoDouble(dealSymbol, SYMBOL_TRADE_TICK_SIZE);
      double pips     = (tickVal > 0 && volume > 0)
                        ? (profit / (volume * tickVal)) * tickSize : 0;

      totalTrades++;

      if(profit >= 0){
         winners++;
         grossProfit  += profit;
         pipsTotalWin += pips;
         consecLoss    = 0;
      } else {
         losers++;
         grossLoss     += MathAbs(profit);
         pipsTotalLoss += MathAbs(pips);
         consecLoss    += MathAbs(profit);
         if(consecLoss > maxConsecLoss) maxConsecLoss = consecLoss;
      }

      runningPnL += profit;
      if(runningPnL > peakPnL) peakPnL = runningPnL;

      double dd = peakPnL - runningPnL;
      if(dd > maxDrawdown) maxDrawdown = dd;
      if(dd > 0){ drawdownSum += dd; drawdownCount++; }
   }

   double netProfit   = grossProfit - grossLoss;
   double winrate     = (totalTrades > 0) ? (double)winners / totalTrades * 100.0 : 0;
   double avgWin      = (winners > 0) ? grossProfit / winners : 0;
   double avgLoss     = (losers  > 0) ? grossLoss   / losers  : 0;
   double expectancy  = (avgWin * (winrate/100.0)) - (avgLoss * (1.0 - winrate/100.0));
   double avgPips     = (totalTrades > 0) ? (pipsTotalWin - pipsTotalLoss) / totalTrades : 0;
   double avgPipsWin  = (winners > 0) ? pipsTotalWin  / winners : 0;
   double avgPipsLoss = (losers  > 0) ? pipsTotalLoss / losers  : 0;
   double avgDD       = (drawdownCount > 0) ? drawdownSum / drawdownCount : 0;
   double maxDDpct    = (peakPnL > 0) ? maxDrawdown / peakPnL * 100.0 : 0;
   double avgDDpct    = (peakPnL > 0) ? avgDD / peakPnL * 100.0 : 0;
   double balance     = AccountInfoDouble(ACCOUNT_BALANCE);
   double totalPct    = (balance > 0) ? netProfit / balance * 100.0 : 0;

   string scope   = InpGlobalHistory ? "CUENTA GLOBAL" : _Symbol;
   string fromStr = TimeToString(g_from, TIME_DATE);
   string toStr   = TimeToString(g_to,   TIME_DATE);

   // ===================== RENDER =====================
   int L = 0;

   DrawLabel("T0",  L++, "  ESTADISTICAS " + scope,                               CLR_TITLE);
   DrawLabel("T1",  L++, "  " + fromStr + " -> " + toStr,                         CLR_LABEL);
   DrawSeparator("S0", L++);

   DrawLabel("S1H", L++, "  [ RENDIMIENTO ]",                                     CLR_SECTION);
   DrawLabel("L01", L++, Pad("  Gan/Perd (%)")      + DoubleToString(totalPct,    2) + "%", ValueColor(totalPct));
   DrawLabel("L02", L++, Pad("  Pips prom/op")      + DoubleToString(avgPips,     1),        ValueColor(avgPips));
   DrawLabel("L03", L++, Pad("  Pips prom Win")     + DoubleToString(avgPipsWin,  1),        CLR_VALUE_POS);
   DrawLabel("L04", L++, Pad("  Pips prom Loss")    + "-" + DoubleToString(avgPipsLoss, 1),  CLR_VALUE_NEG);
   DrawLabel("L05", L++, Pad("  DD promedio (%)")   + DoubleToString(avgDDpct,    2) + "%",  ValueColor(-avgDDpct));
   DrawLabel("L06", L++, Pad("  DD max hist (%)")   + DoubleToString(maxDDpct,    2) + "%",  ValueColor(-maxDDpct));
   DrawSeparator("S1", L++);

   DrawLabel("S2H", L++, "  [ TRADING ]",                                         CLR_SECTION);
   DrawLabel("L07", L++, Pad("  Trades totales")    + IntegerToString(totalTrades),            CLR_VALUE_NEU);
   DrawLabel("L08", L++, Pad("  Ganadores")         + IntegerToString(winners),                CLR_VALUE_POS);
   DrawLabel("L09", L++, Pad("  Perdedores")        + IntegerToString(losers),                 CLR_VALUE_NEG);
   DrawLabel("L10", L++, Pad("  Winrate (%)")       + DoubleToString(winrate,     1) + "%",    ValueColor(winrate - 50));
   DrawLabel("L11", L++, Pad("  Avg Win $")         + DoubleToString(avgWin,      2),          CLR_VALUE_POS);
   DrawLabel("L12", L++, Pad("  Avg Loss $")        + "-" + DoubleToString(avgLoss, 2),        CLR_VALUE_NEG);
   DrawLabel("L13", L++, Pad("  Expectancy $")      + DoubleToString(expectancy,  2),          ValueColor(expectancy));
   DrawLabel("L14", L++, Pad("  Max consec loss $") + DoubleToString(maxConsecLoss, 2),        CLR_VALUE_NEG);
   DrawSeparator("S2", L++);

   DrawLabel("S3H", L++, "  [ FINANCIERO ]",                                      CLR_SECTION);
   DrawLabel("L15", L++, Pad("  Profit total $")    + DoubleToString(grossProfit,  2),         CLR_VALUE_POS);
   DrawLabel("L16", L++, Pad("  Loss total $")      + "-" + DoubleToString(grossLoss, 2),      CLR_VALUE_NEG);
   DrawLabel("L17", L++, Pad("  Neto $")            + DoubleToString(netProfit,    2),         ValueColor(netProfit));
   DrawLabel("L18", L++, Pad("  Balance actual $")  + DoubleToString(balance,      2),         CLR_VALUE_NEU);
   DrawSeparator("S3", L++);

   // Resize panel por si cambia la escala en caliente
   ObjectSetInteger(0, g_prefix+"BG", OBJPROP_XSIZE, g_panelW);
   ObjectSetInteger(0, g_prefix+"BG", OBJPROP_YSIZE, g_panelH);

   ChartRedraw(0);
}



