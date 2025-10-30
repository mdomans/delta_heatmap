#include "sierrachart.h"
#include <deque>
#include <vector>
#include <algorithm>
#include <cmath>

SCDLLName("BidAsk Weighted Percentile Number Bars")

SCSFExport scsf_BidAskWeightedPercentileNumberBars(SCStudyInterfaceRef sc)
{
    SCSubgraphRef Subgraph_NumberBar = sc.Subgraph[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Bid/Ask Weighted Percentile Number Bars";
        sc.AutoLoop = 1;
        sc.GraphRegion = 1; // separate panel below price

        Subgraph_NumberBar.Name = "BidAsk Weighted Number Bar";
        Subgraph_NumberBar.DrawStyle = DRAWSTYLE_BAR;
        Subgraph_NumberBar.PrimaryColor = RGB(0, 128, 255);   // blue for positive
        Subgraph_NumberBar.SecondaryColor = RGB(255, 0, 0);   // red for negative
        Subgraph_NumberBar.DrawZeros = true;
        Subgraph_NumberBar.LineWidth = 3;
        Subgraph_NumberBar.SecondaryColorUsed = true;

        sc.Input[0].Name = "Lookback Bars";
        sc.Input[0].SetInt(2000);
        sc.Input[0].SetIntLimits(50, 10000);

        sc.Input[1].Name = "Recency Weight Alpha";
        sc.Input[1].SetFloat(0.05f);
        sc.Input[1].SetFloatLimits(0.001f, 0.5f);

        return;
    }

    int lookbackBars = sc.Input[0].GetInt();
    float alpha = sc.Input[1].GetFloat();
    static std::deque<float> imbalanceHistory;

    float bidVol = sc.BaseData[SC_BIDVOL][sc.Index];
    float askVol = sc.BaseData[SC_ASKVOL][sc.Index];
    float totalVol = bidVol + askVol;

    if (totalVol <= 0)
    {
        Subgraph_NumberBar[sc.Index] = 0.0f;
        return;
    }

    float imbalance = (askVol - bidVol) / totalVol;

    imbalanceHistory.push_back(imbalance);
    if ((int)imbalanceHistory.size() > lookbackBars)
        imbalanceHistory.pop_front();

    if ((int)imbalanceHistory.size() < 10)
    {
        Subgraph_NumberBar[sc.Index] = 0.0f;
        return;
    }

    // Calculate recency-weighted percentile
    std::vector<std::pair<float, float>> weightedHist;
    int histSize = (int)imbalanceHistory.size();
    float totalWeight = 0.0f;
    for (int i = 0; i < histSize; ++i)
    {
        float w = alpha * powf(1.0f - alpha, histSize - 1 - i);
        weightedHist.push_back({ imbalanceHistory[i], w });
        totalWeight += w;
    }

    std::sort(weightedHist.begin(), weightedHist.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    float cumWeight = 0.0f;
    for (int i = 0; i < histSize; ++i)
    {
        if (weightedHist[i].first < imbalance)
            cumWeight += weightedHist[i].second;
        else
            break;
    }
    float weightedPercentile = cumWeight / totalWeight;

    // Map weighted percentile [0..1] to integer number bar [-4..4]
    int intValue = (int)roundf((weightedPercentile - 0.5f) * 8.0f);
    Subgraph_NumberBar[sc.Index] = (float)intValue;
}
