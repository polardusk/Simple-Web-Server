#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "server_http.hpp"

//Added for the default_resource example
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
//Added for the json-example:

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

void StartServer(char *ip, int port, char *webDir)
{
	HttpServer server(port, 4);
    
    //Add resources using path-regex and method-string, and an anonymous function
    //POST-example for the path /string, responds the posted string
    server.resource["^/string$"]["POST"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        //Retrieve string:
        auto content=request->content.string();
        //request->content.string() is a convenience function for:
        //stringstream ss;
        //ss << request->content.rdbuf();
        //string content=ss.str();
        
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };
        
    //GET-example for the path /info
    //Responds with request-information
    server.resource["^/info$"]["GET"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        stringstream content_stream;
        content_stream << "<h1>Request from " << request->remote_endpoint_address << " (" << request->remote_endpoint_port << ")</h1>";
        content_stream << request->method << " " << request->path << " HTTP/" << request->http_version << "<br>";
        for(auto& header: request->header) {
            content_stream << header.first << ": " << header.second << "<br>";
        }
        
        //find length of content_stream (length received using content_stream.tellp())
        content_stream.seekp(0, ios::end);
        
        response <<  "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp() << "\r\n\r\n" << content_stream.rdbuf();
    };
    
    //GET-example for the path /match/[number], responds with the matched string in path (number)
    //For instance a request GET /match/123 will receive: 123
    server.resource["^/match/([0-9]+)$"]["GET"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        string number=request->path_match[1];
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << number.length() << "\r\n\r\n" << number;
    };
    
    //Default GET-example. If no other matches, this anonymous function will be called. 
    //Will respond with content in the web/-directory, and its subdirectories.
    //Default file: index.html
    //Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
    server.default_resource["GET"]=[&webDir](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        boost::filesystem::path web_root_path(webDir);
        if(!boost::filesystem::exists(web_root_path))
            cerr << "Could not find web root." << endl;
        else {
            auto path=web_root_path;
            path+=request->path;
            if(boost::filesystem::exists(path)) {
                if(boost::filesystem::canonical(web_root_path)<=boost::filesystem::canonical(path)) {
                    if(boost::filesystem::is_directory(path))
                        path+="/index.html";
                    if(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path)) {
                        ifstream ifs;
                        ifs.open(path.string(), ifstream::in | ios::binary);
                        
                        if(ifs) {
                            ifs.seekg(0, ios::end);
                            size_t length=ifs.tellg();
                            
                            ifs.seekg(0, ios::beg);
                            
                            response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";
                            
                            //read and send 128 KB at a time
                            size_t buffer_size=131072;
                            vector<char> buffer;
                            buffer.reserve(buffer_size);
                            size_t read_length;
                            try {
                                while((read_length=ifs.read(&buffer[0], buffer_size).gcount())>0) {
                                    response.write(&buffer[0], read_length);
                                    response.flush();
                                }
                            }
                            catch(const exception &e) {
                                cerr << "Connection interrupted, closing file" << endl;
                            }

                            ifs.close();
                            return;
                        }
                    }
                }
            }
        }
        string content="Could not open path "+request->path;
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };
    
    //Start server
    server.start();
}

int main(int argc, char **argv)
{
	char* ip;
	char* dir;
	int	  port;

	int rez = 0;

	while ((rez = getopt(argc, argv, "h:p:d:")) != -1) {
		switch(rez)
		{
			case 'h':
				ip = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				dir = optarg;
				break;
		}
	}

	if (fork() == 0)
	{
		setsid();
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		StartServer(ip, port, dir);
	}
	return 0;
}
