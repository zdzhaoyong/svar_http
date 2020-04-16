#include "httplib.h"
#include <thread>
#include <Svar/Svar.h>

using namespace sv;
using namespace std;
using httplib::Request;
using httplib::Response;

class Server{
public:
    Server(std::string host,int port,Svar api)
        : host(host),port(port),api(api){

        server.Get("/[^.]*",[this](const Request& req,Response& res){
            get(req,res);
        });
        server.Post("/[^.]*",[this](const Request& req,Response& res){
            get(req,res);
        });

        worker=std::thread([this](){listen_thread();});
    }

    ~Server(){
        server.stop();
        worker.join();
    }

    void toRespose(Svar& ret,Response& res){
        if(ret.is<SvarBuffer>()){
            SvarBuffer& buf=ret.as<SvarBuffer>();
            res.body=std::string();
            res.set_content((char*)buf.ptr(),buf.size(),
                            "application/x-www-form-urlencoded");
        }
        else{
            res.set_content(ret.dump_json(),"application/json");
        }
    }

    void get(const Request& req,Response& res){
        Svar ret=call(req.path,parameter(req),Svar());
        toRespose(ret,res);
    }

    void post(const Request& req,Response& res){
        Svar ret=call(req.path,parameter(req),body(req));
        toRespose(ret,res);
    }

    Svar parameter(const Request& req){
        Svar ret=Svar::object();
        for(auto it:req.params){
            if(ret[it.first].isUndefined())
                ret[it.first]=it.second;
            else if(ret[it.first].is<std::string>())
                ret[it.first]=Svar::array({ret[it.first],it.second});
            else
                ret[it.first].push_back(it.second);
        }
        return ret;
    }

    static Svar body(const Request& req){
        std::string type=req.get_header_value("Content-Type");
        if(type=="application/json")
            return Svar::parse_json(req.body);
        else{
            return {{"Content-Type",type},
                    {"body",SvarBuffer(req.body.data(),req.body.size())}};
        }
    }

    Svar call(std::string path,Svar parameters,Svar body){
        if(path=="/")
            return api;
        else
            path=path.substr(1);

        for(char& c:path)
            if(c=='/') c='.';

        Svar func=api.get(path,Svar(),true);
        if(func.isUndefined())
            return {{"code","NotFound"}};

        if(func.isFunction()){
            try{
                if(body.isUndefined())
                    return func(parameters);
                else
                    return func(parameters,body);
            }
            catch(SvarExeption& e){
                return {{"code","SvarException"},{"message",e.what()}};
            }
            catch(std::exception& e){
                return {{"code","CXXException"},{"message",e.what()}};
            }
        }

        return func;
    }

    void listen_thread(){
        std::cout<<"Listening "<<host<<":"<<port<<" ..."<<std::endl;
        server.listen(host.c_str(),port);
        std::cout<<"Closed "<<host<<":"<<port<<"."<<std::endl;
    }

    std::string     host;
    int             port;
    Svar            api;
    std::thread     worker;
    httplib::Server server;
};

class HTTPURL
{
    private:
        string _protocol;// http vs https
        string _domain;  // mail.google.com
        uint16_t _port;  // 80,443
        string _path;    // /mail/
        string _query;   // [after ?] a=b&c=b

    public:
        const string &protocol;
        const string &domain;
        const uint16_t &port;
        const string &path;
        const string &query;

        HTTPURL(const string& url): protocol(_protocol),domain(_domain),port(_port),path(_path),query(_query)
        {
            string u = _trim(url);
            size_t offset=0, slash_pos, hash_pos, colon_pos, qmark_pos;
            string urlpath,urldomain,urlport;
            uint16_t default_port;

            static const char* allowed[] = { "https://", "http://", "ftp://", NULL};
            for(int i=0; allowed[i]!=NULL && this->_protocol.length()==0; i++)
            {
                const char* c=allowed[i];
                if (u.compare(0,strlen(c), c)==0) {
                    offset = strlen(c);
                    this->_protocol=string(c,0,offset-3);
                }
            }
            default_port = this->_protocol=="https" ? 443 : 80;
            slash_pos = u.find_first_of('/', offset+1 );
            urlpath = slash_pos==string::npos ? "/" : u.substr(slash_pos);
            urldomain = string( u.begin()+offset, slash_pos != string::npos ? u.begin()+slash_pos : u.end() );
            urlpath = (hash_pos = urlpath.find("#"))!=string::npos ? urlpath.substr(0,hash_pos) : urlpath;
            urlport = (colon_pos = urldomain.find(":"))!=string::npos ? urldomain.substr(colon_pos+1) : "";
            urldomain = urldomain.substr(0, colon_pos!=string::npos ? colon_pos : urldomain.length());
            this->_domain = _tolower(urldomain);
            this->_query = (qmark_pos = urlpath.find("?"))!=string::npos ? urlpath.substr(qmark_pos+1) : "";
            this->_path = qmark_pos!=string::npos ? urlpath.substr(0,qmark_pos) : urlpath;
            this->_port = urlport.length()==0 ? default_port : _atoi(urlport) ;
        };

        static std::shared_ptr<Response> Get(const std::string& url){
            HTTPURL u(url);
            httplib::Client client(u.domain.c_str(),u.port);
            auto query=u.query.empty()?"":("?"+u.query);
            return client.Get((u.path+query).c_str());
        }

        static std::shared_ptr<Response> Post(const std::string& url, const std::string &body,
                                              const char *content_type="application/json"){
            HTTPURL u(url);
            httplib::Client client(u.domain.c_str(),u.port);
            auto query=u.query.empty()?"":("?"+u.query);
            return client.Post((u.path+query).c_str(),body,content_type);
        }

    private:
        static inline string _trim(const string& input)
        {
            string str = input;
            size_t endpos = str.find_last_not_of(" \t\n\r");
            if( string::npos != endpos )
            {
                str = str.substr( 0, endpos+1 );
            }
            size_t startpos = str.find_first_not_of(" \t\n\r");
            if( string::npos != startpos )
            {
                str = str.substr( startpos );
            }
            return str;
        };
        static inline string _tolower(const string& input)
        {
            string str = input;
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        };
        static inline int _atoi(const string& input)
        {
            int r;
            std::stringstream(input) >> r;
            return r;
        };
};

std::string md5(Svar paras){
    std::string str=paras["str"].castAs<std::string>();
    return SvarBuffer(str.data(),str.size()).md5();
}

Svar http_get(std::string url){
    auto res=HTTPURL::Get(url);
    if(!res)
        return Svar();
    std::string type=res->get_header_value("Content-Type");
    if(type=="application/json")
        return Svar::parse_json(res->body);
    else{
        return {{"Content-Type",type},
            {"body",SvarBuffer(res->body.data(),res->body.size())}};
    }
}

Svar http_post(std::string url,Svar req){
    std::string bodyStr;
    std::string bodyType="application/json";
    Svar body=req["body"];
    if(!body.is<SvarBuffer>())
        bodyStr=req.dump_json();
    else
    {
        SvarBuffer& buf=body.as<SvarBuffer>();
        bodyStr=std::string((char*)buf.ptr(),buf.size());
        bodyType=req["type"].as<std::string>();
    }
    auto res=HTTPURL::Post(url,bodyStr,bodyType.c_str());
    if(!res)
        return Svar();
    std::string type=res->get_header_value("Content-Type");
    if(type=="application/json")
        return Svar::parse_json(res->body);
    else{
        return {{"Content-Type",type},
            {"body",SvarBuffer(res->body.data(),res->body.size())}};
    }
}

int main(int argc,char** argv){
    svar["md5"]=md5;
    Server server("0.0.0.0",1234,svar);

    sleep(1000);
    return 0;
}

REGISTER_SVAR_MODULE(http){
    svar["get"] =http_get;
    svar["post"]=http_post;

    Class<Server>("Server")
            .def("__init__",[](std::string host,int port,Svar api){
        return std::make_shared<Server>(host,port,api);
    })
            .def_readonly("port",&Server::port);
}

EXPORT_SVAR_INSTANCE
