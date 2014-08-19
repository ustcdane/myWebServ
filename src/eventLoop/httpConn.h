#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "epoller.h"
#include "eventLoop.h"

namespace myWebServ
{

class eventLoop;

class httpConn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP���󷽷�
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    // ����HTTP����ʱ,��״̬������״̬
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    // ����������HTTP����Ŀ���״̬
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    // �еĶ�ȡ״̬
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    httpConn(int sockfd, epoller *pep, eventLoop *lp):m_sockfd(sockfd),pEpoll(pep), pLoop(lp)
    {
    	int error = 0;
    	socklen_t len = sizeof( error );
    	getsockopt( m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len );
    	int reuse = 1;
    	// �ر�TIME_WAIT״̬
    	setsockopt( m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
	init();
    	pEpoll->add(sockfd);
    }

    ~httpConn()
    {
        close(m_sockfd);
    }

public:
    void process();
    bool read();
    bool write();

private:
    void init();// ��ʼ������
    HTTP_CODE process_read();// ����HTTP����
    bool process_write( HTTP_CODE ret );// ���HTTPӦ��

    // process_read()�������溯������HTTP�������
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // process_write() ��������ĺ������ HTTPӦ��
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

private:
    epoller *pEpoll;// ����socket�¼�ע�ᵽ���epoll�ں��¼���
    eventLoop *pLoop;

    int m_sockfd;// ��HTTP���ӵ�socket
    char m_read_buf[ READ_BUFFER_SIZE ];// �Ȼ�����
    int m_read_idx;// ��ʶ�������������ֽڵ�λ��
    int m_checked_idx;// ��ǰ���ڷ������ַ��ڻ�������λ��
    int m_start_line;// ��ǰ���ڽ������е���ʼλ��
    char m_write_buf[ WRITE_BUFFER_SIZE ];// д������
    int m_write_idx;// д�������������ֽ���
    CHECK_STATE m_check_state; // ��״̬������״̬
    METHOD m_method;// ���󷽷�
    char m_real_file[ FILENAME_LEN ];// �ͻ������Ŀ���ļ�������·��
    char* m_url;// �ͻ������Ŀ���ļ����ļ���
    char* m_version;// HTTPЭ��汾
    char* m_host;// ������
    int m_content_length;// HTTP�������Ϣ��ĳ���
    bool m_linger;// HTTP�����Ƿ񱣳����� keeplive
    char* m_file_address;// �ͻ�����mmap���ڴ��е���ʼλ��
    struct stat m_file_stat;// Ŀ���ļ���״̬
    struct iovec m_iv[2];// ���� writev��ִ��д����������ı���
    int m_iv_count;
};

}
#endif
