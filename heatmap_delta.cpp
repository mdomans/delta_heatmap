#include "sierrachart.h"
#include <deque>
#include <vector>
#include <algorithm>

SCDLLName("BidAsk Percentile Bar Coloring")

SCSFExport scsf_BidAskPercentileColoring(SCStudyInterfaceRef sc)
{
    SCSubgraphRef Subgraph_BarColor = sc.Subgraph[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Bid/Ask Percentile Bar Coloring";
        sc.AutoLoop = 1; // run per bar
        sc.GraphRegion = 0;

        Subgraph_BarColor.Name = "Bar Color";
        Subgraph_BarColor.DrawStyle = DRAWSTYLE_COLOR_BAR;
        Subgraph_BarColor.PrimaryColor = RGB(128, 128, 128);

        sc.Input[0].Name = "Lookback Bars";
        sc.Input[0].SetInt(2000);
        sc.Input[0].SetIntLimits(50, 10000);

        sc.Input[1].Name = "Neutral Zone (Percent)";
        sc.Input[1].SetFloat(0.0f); // e.g., 0.2 = 20% middle range is gray
        sc.Input[1].SetFloatLimits(0.0f, 0.5f);

        return;
    }

    int lookbackBars = sc.Input[0].GetInt();
    float neutralZone = sc.Input[1].GetFloat();

    // Static buffer to persist data between calls
    static std::deque<float> imbalanceHistory;

    // Get bid/ask volumes for this bar
    float bidVol = sc.BaseData[SC_BIDVOL][sc.Index];
    float askVol = sc.BaseData[SC_ASKVOL][sc.Index];
    float totalVol = bidVol + askVol;

    if (totalVol <= 0)
    {
        Subgraph_BarColor.DataColor[sc.Index] = RGB(128, 128, 128);
        return;
    }

    // Imbalance = -1 (all bid) ... +1 (all ask)
    float imbalance = (askVol - bidVol) / totalVol;

    // Store imbalance in history
    imbalanceHistory.push_back(imbalance);
    if ((int)imbalanceHistory.size() > lookbackBars)
        imbalanceHistory.pop_front();

    // Need enough data before percentile calculation
    if ((int)imbalanceHistory.size() < 10)
    {
        Subgraph_BarColor.DataColor[sc.Index] = RGB(128, 128, 128);
        return;
    }

    // Calculate percentile rank
    std::vector<float> sortedHist(imbalanceHistory.begin(), imbalanceHistory.end());
    std::sort(sortedHist.begin(), sortedHist.end());

    int rank = (int)(std::lower_bound(sortedHist.begin(), sortedHist.end(), imbalance) - sortedHist.begin());
    float percentile = (float)rank / (float)sortedHist.size();

    // Apply neutral zone
    float lowerNeutral = 0.5f - neutralZone / 2.0f;
    float upperNeutral = 0.5f + neutralZone / 2.0f;

    if (percentile >= lowerNeutral && percentile <= upperNeutral)
    {
        Subgraph_BarColor.DataColor[sc.Index] = RGB(128, 128, 128); // neutral gray
        return;
    }

    // Map percentile to red-green gradient
    int red   = (int)((1.0f - percentile) * 255);
    int green = (int)(percentile * 255);
    int blue  = 0;

    Subgraph_BarColor.DataColor[sc.Index] = RGB(red, green, blue);
}

