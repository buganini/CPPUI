#ifndef CPPUI_COOKBOOK_HEAVYCANVAS_DATA
#define CPPUI_COOKBOOK_HEAVYCANVAS_DATA
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Cookbook { namespace HeavyCanvasData {
    inline const std::vector<int> & sampleCounts() {
        static const std::vector<int> counts{1000,3000,5000,7500,10000,15000,20000,25000,30000,
            120000,250000,350000,500000,1000000};
        return counts;
    }
    struct Row {
        std::string label;
        double open, high, low, close, volume, mid, upper, lower, k, d, rsi;
    };
    struct ChartData { std::vector<Row> rows; };
    inline std::vector<std::string> sampleLabels(int count) {
        std::vector<std::string> result;
        result.reserve(count+20);
        if(count>500000) {
            for(int i=-20;i<count;++i) {
                std::string label=std::to_string(i);
                for(int pos=int(label.size())-3;pos>0 && label[pos-1]!='-';pos-=3) label.insert(pos,",");
                result.push_back(std::move(label));
            }
            return result;
        }
        int year=2026, month=10, day=13, weekday=1;
        const int lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
        while(int(result.size())<count+20) {
            if(weekday<5) {
                std::ostringstream out; out << std::setfill('0') << std::setw(4) << year << '-' << std::setfill('0') << std::setw(2) << month << '-' << std::setw(2) << day;
                result.push_back(out.str());
            }
            if(--day==0) {
                if(--month==0) { month=12; --year; }
                day=lengths[month-1]+(month==2 && year%4==0 && (year%100!=0 || year%400==0));
            }
            weekday=(weekday+6)%7;
        }
        std::reverse(result.begin(),result.end()); return result;
    }
    inline std::shared_ptr<ChartData> generate(int count,int seed) {
        auto data=std::make_shared<ChartData>(); data->rows.reserve(count+20);
        std::mt19937 rng(seed); std::normal_distribution<double> normal(0,1);
        std::uniform_real_distribution<double> range(.001,.012);
        std::lognormal_distribution<double> volume(17,.45);
        double close=320,k=50,d=50,gain=0,loss=0;
        for(const auto & label:sampleLabels(count)) {
            Row row{}; row.label=label; row.open=close*std::exp(normal(rng)*.003);
            row.close=row.open*std::exp(1.7/count+normal(rng)*.014);
            row.high=std::max(row.open,row.close)*(1+range(rng)); row.low=std::min(row.open,row.close)*(1-range(rng));
            row.volume=std::floor(volume(rng)); data->rows.push_back(row);
            int i=int(data->rows.size())-1, first=std::max(0,i-19); double mid=0,sigma=0;
            for(int j=first;j<=i;++j) mid+=data->rows[j].close;
            mid/=i-first+1;
            for(int j=first;j<=i;++j) sigma+=std::pow(data->rows[j].close-mid,2);
            sigma=std::sqrt(sigma/(i-first+1)); double low=row.low,high=row.high;
            for(int j=std::max(0,i-8);j<i;++j) { low=std::min(low,data->rows[j].low); high=std::max(high,data->rows[j].high); }
            k=(2*k+100*(row.close-low)/(high-low))/3; d=(2*d+k)/3;
            if(i) { double change=row.close-close; int period=std::min(i,14); gain+=(std::max(change,0.0)-gain)/period; loss+=(std::max(-change,0.0)-loss)/period; }
            auto & current=data->rows.back(); current.mid=mid; current.upper=mid+2*sigma; current.lower=mid-2*sigma;
            current.k=k; current.d=d; current.rsi=gain+loss ? 100*gain/(gain+loss) : 50;
            close=row.close;
        }
        data->rows.erase(data->rows.begin(),data->rows.begin()+20); return data;
    }
}}
#endif
