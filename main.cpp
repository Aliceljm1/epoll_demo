#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>


using namespace std;

int TIMEOUT_MS=6000;
int SERVER_PORT=8888;

void perror_and_exit(const char * szErrorInfo){
	printf("errno=%d,", errno);
	perror(szErrorInfo);
	exit(1);
}

int main( int argc, char *argv[] ){
	int epfd = epoll_create(256);
	if( epfd == -1 ){
		perror_and_exit("epoll_create failed");
	}

    int server_listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    if( server_listenfd == -1 ){
		perror_and_exit("socket() failed");
	}

	{
		struct epoll_event ev;
		ev.events = EPOLLIN|EPOLLET;
        ev.data.fd = server_listenfd;
        // 新增链接的监听函数,可读|边沿触发
        cout<<"init server fd="<<server_listenfd<<endl;
        epoll_ctl( epfd, EPOLL_CTL_ADD, server_listenfd, &ev );
	}

	// listening on the port
	{
        //初始化链接地址
		struct sockaddr_in serveraddr;
		bzero( &serveraddr, sizeof(serveraddr) );
		serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(SERVER_PORT);
        const char *local_ip = "0.0.0.0";
        inet_aton( local_ip, &serveraddr.sin_addr );//配置ip地址
        // 将socket绑定到地址上去
        bind( server_listenfd, (sockaddr*)&serveraddr, sizeof(serveraddr) );
        listen( server_listenfd, 10 );//开始监听
	}

	static const int MAX_READY_EVENTS = 256;
	struct epoll_event events[MAX_READY_EVENTS];
	while (true){
		printf("working...\n");
        int nfds = epoll_wait( epfd, events, MAX_READY_EVENTS, TIMEOUT_MS );
        // 开始等待事件发生,如果返回0则表示超时,如果不超时则返回需要处理的事件数目,
        printf("eploo_wait's return value ntds=%d\n", nfds);
		for( int nIndex = 0; nIndex<nfds; ++nIndex ){
			struct epoll_event & active_event = events[nIndex];

            if( active_event.data.fd == server_listenfd ) {
                // 当一个客户端链接过来的时候执行如下代码
                cout<<"accepting new client..."<<endl;

				struct sockaddr_in clientaddr;
				socklen_t clilen = sizeof(clientaddr);
                int connfd = accept( server_listenfd, (sockaddr*)&clientaddr, &clilen );
                // 获取客户端链接过来产生的fd,以后客户端发送的消息都是这个fd发送的.
				if( connfd<0 ){
					perror_and_exit("connfd<0");
				}

				char * remote_ip = inet_ntoa( clientaddr.sin_addr);
                cout<<"accept a connection from " << remote_ip<<"client's fd="<<connfd<<endl;

                //绑定fd事件
				epoll_event clientEv;
				clientEv.data.fd = connfd;
				clientEv.events = EPOLLIN|EPOLLET;
				int add_result = epoll_ctl( epfd, EPOLL_CTL_ADD, connfd, &clientEv);
				if ( -1==add_result ){
					perror_and_exit( "EPOLL_CTL_ADD failed" );
				}



			}
			else if( active_event.events & EPOLLIN ){
                int client_link_fd=active_event.data.fd;
                cout<<"EPOLLIN event emit,fd="<<client_link_fd<<endl;

				char line[1024]={0};
                int nReaded = read( client_link_fd, line, sizeof(line)-1 );
				if( nReaded <= 0 ){
                    // 客户端传递的内容为空,说明关闭了,
					perror("nReaded<=0");

					if ( nReaded == 0 ){
						printf("client closed gracefully!\n");
					}
                    //删除事件
                    epoll_ctl( epfd, EPOLL_CTL_DEL, client_link_fd, 0 );
                    close( client_link_fd );//关闭链接
				}
                else{
                    // 打印出客户端传递的内容
					cout<<"Readed: ["<<line<<"]"<<endl;
                    char* ok="getmsg_ok";
                    int nSended = send( client_link_fd, ok, strlen(ok), 0 );
                    // 返回内容
					if( -1==nSended ){
						perror( "send" );
						printf("errno=%d\n", errno );
						if( errno == EAGAIN ){
							printf("It's time to do something. the buffer is not sent.\n");
						}
					}
				}

			}
			else{
				printf("errno=%d, unkown event type!\n", errno );
			}
			
		}
	}


	return 0;
}

