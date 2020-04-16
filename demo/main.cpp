#include <Svar/Registry.h>

using namespace sv;

auto http=Registry::load("svar_http");

std::string md5(Svar paras){
    std::string str=paras["str"].castAs<std::string>();
    std::cerr<<paras<<std::endl<<str<<std::endl;
    std::string ret=SvarBuffer(str.data(),str.size()).clone().md5();
    return ret;
}

double sum(Svar paras){
    double x=std::stod(paras["x"].as<std::string>());
    double y=std::stod(paras["y"].as<std::string>());

    return x+y;
}

int main(int argc,char** argv){
    svar["md5"]=md5;
    svar["sum"]=sum;

    auto Server=http["Server"]("0.0.0.0",1234,svar);

    sleep(1000);
    return 0;
}
