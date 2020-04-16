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
    return paras["x"].castAs<double>()+paras["y"].castAs<double>();
}

int main(int argc,char** argv){
    svar["md5"]=md5;
    svar["sum"]=sum;

    Class<std::string>()
            .def("__double__",[](std::string& str){return std::stod(str);});

    auto Server=http["Server"]("0.0.0.0",1234,svar);

    sleep(1000);
    return 0;
}
