#include "../cookbook/heavycanvas_data.hpp"
#include <iostream>
#include <stdexcept>
using namespace Cookbook::HeavyCanvasData;
static void require(bool value,const char * message){if(!value)throw std::runtime_error(message);}
int main(){
    try {
        require(sampleCounts()==std::vector<int>({1000,3000,5000,7500,10000,15000,20000,25000,30000,
            120000,250000,350000,500000,1000000}),"native presets differ from PUI");
        auto dates=sampleLabels(500000);
        require(dates.size()==500020&&dates.front()=="0110-03-05"&&dates[20]=="0110-04-02"
            &&dates.back()=="2026-10-13"&&std::is_sorted(dates.begin(),dates.end()),"large date labels lost ISO format or range");
        for(int count:{500001,1000000}){
            auto labels=sampleLabels(count);
            require(labels.size()==std::size_t(count+20)&&labels[0]=="-20"&&labels[19]=="-1"
                &&labels[20]=="0"&&labels[21]=="1"&&labels[1020]=="1,000"
                &&labels.back()==(count==500001?"500,000":"999,999"),"index labels lost warmup offset or grouping");
        }
        auto first=generate(1000,42),same=generate(1000,42),other=generate(1000,43);
        require(first->rows.front().close==same->rows.front().close
            &&first->rows.front().close!=other->rows.front().close,"seeded generation is not repeatable");
        auto data=generate(1000000,42);
        require(data->rows.size()==1000000&&data->rows.front().label=="0"&&data->rows.back().label=="999,999",
            "million-row generation lost visible range");
        for(const auto & row:data->rows){
            require(row.low<std::min(row.open,row.close)&&row.high>std::max(row.open,row.close)&&row.volume>0,
                "invalid OHLCV values");
            for(double value:{row.open,row.high,row.low,row.close,row.volume,row.mid,row.upper,row.lower,row.k,row.d,row.rsi})
                require(std::isfinite(value),"million-row indicator became non-finite");
            require(row.k>=0&&row.k<=100&&row.d>=0&&row.d<=100&&row.rsi>=0&&row.rsi<=100,
                "indicator escaped its range");
        }
        double mean=0;for(std::size_t i=data->rows.size()-20;i<data->rows.size();++i)mean+=data->rows[i].close;
        require(std::abs(mean/20-data->rows.back().mid)<1e-9,"warmup or band calculation changed");
        std::cout<<"HeavyCanvas million-row data passed\n";
    }catch(const std::exception & e){std::cerr<<e.what()<<'\n';return 1;}
}
