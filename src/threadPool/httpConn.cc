// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)


#include "httpConn.h"

namespace myWebServ
{

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
// web服务器html网页的根目录
const char* doc_root = "/home/ubuntu/myWebServ/src/threadPool";

int httpConn::epoll_fd = -1;

void httpConn::close_conn( bool real_close )
{
    if( real_close && ( sock_fd != -1 ) )
    {
        //modfd( epoll_fd, sock_fd, EPOLLIN );
        removefd( epoll_fd, sock_fd );
        sock_fd = -1;
    }
}

void httpConn::init( int sockfd, const sockaddr_in& addr )
{
    sock_fd = sockfd;
    m_address = addr;
    int error = 0;
    socklen_t len = sizeof( error );
    getsockopt( sock_fd, SOL_SOCKET, SO_ERROR, &error, &len );
    int reuse = 1;
    // 关闭TIME_WAIT状态
    setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    addfd( epoll_fd, sockfd, true );

    init();
}

void httpConn::init()
{
    check_state = CHECK_STATE_REQUESTLINE;
    linger_ = false;

    method_ = GET;
    url_ = 0;
    version_ = 0;
    content_length = 0;
    host_ = 0;
    start_line = 0;
    checked_index = 0;
    read_index = 0;
    write_index = 0;
    memset( read_buf, '\0', READ_BUFFER_SIZE );
    memset( write_buf, '\0', WRITE_BUFFER_SIZE );
    memset( real_file, '\0', FILENAME_LEN );
}
// 从状态机
httpConn::LINE_STATUS httpConn::parse_line()
{
    char temp;
    for ( ; checked_index < read_index; ++checked_index )
    {
        temp = read_buf[ checked_index ];
        if ( temp == '\r' )
        {
            if ( ( checked_index + 1 ) == read_index )
            {
                return LINE_OPEN;
            }
            else if ( read_buf[ checked_index + 1 ] == '\n' )
            {
                read_buf[ checked_index++ ] = '\0';
                read_buf[ checked_index++ ] = '\0';
                return LINE_OK;
            }

            return LINE_BAD;
        }
        else if( temp == '\n' )
        {
            if( ( checked_index > 1 ) && ( read_buf[ checked_index - 1 ] == '\r' ) )
            {
                read_buf[ checked_index-1 ] = '\0';
                read_buf[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }

    return LINE_OPEN;
}
// 循环读取客户数据，直到无数据或对方关闭连接
bool httpConn::read()
{
    if( read_index >= READ_BUFFER_SIZE )
    {
        return false;
    }

    int bytes_read = 0;
    while( true )// 循环读
    {
        bytes_read = recv( sock_fd, read_buf + read_index, READ_BUFFER_SIZE - read_index, 0 );
        if ( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )// 无数据
            {
                break;
            }
            return false;
        }
        else if ( bytes_read == 0 )// 客户关闭连接
        {
            return false;
        }

        read_index += bytes_read;
    }
    return true;
}
// 解析客户请求行，获得请求方法、url、HTTP协议版本
httpConn::HTTP_CODE httpConn::parse_request_line( char* text )
{
    url_ = strpbrk( text, " \t" );// 遇到' '或'\t'返回
    if ( ! url_ )// 无' '或'\t' 则出错
    {
        return BAD_REQUEST;
    }
    *url_++ = '\0';

    char* method = text;// 得到请求方法 字符串
    if ( strcasecmp( method, "GET" ) == 0 )// 只处理 GET方法
    {
        method_ = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    url_ += strspn( url_, " \t" );// 跳过所有' '或'\t'
    version_ = strpbrk( url_, " \t" );
    if ( ! version_ )
    {
        return BAD_REQUEST;
    }
    *version_++ = '\0';
    version_ += strspn( version_, " \t" );// 得到协议版本
    if ( strcasecmp( version_, "HTTP/1.1" ) != 0 )
    {
        return BAD_REQUEST;
    }
    // 检测URL是否合法
    if ( strncasecmp( url_, "http://", 7 ) == 0 )
    {
        url_ += 7;
        url_ = strchr( url_, '/' );//得到文件路径
    }

    if ( ! url_ || url_[ 0 ] != '/' )
    {
        return BAD_REQUEST;
    }

    check_state = CHECK_STATE_HEADER;// 状态转移到头部字段解析
    return NO_REQUEST;
}
// 解析HTTP请求头部字段
httpConn::HTTP_CODE httpConn::parse_headers( char* text )
{
    if( text[ 0 ] == '\0' )// 遇到空行
    {
        if ( method_ == HEAD )
        {
            return GET_REQUEST;
        }

        if ( content_length != 0 )
        {
            check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }

        return GET_REQUEST;
    }
    else if ( strncasecmp( text, "Connection:", 11 ) == 0 )
    {
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 )
        {
            linger_ = true;
        }
    }
    else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 )
    {
        text += 15;
        text += strspn( text, " \t" );
        content_length = atol( text );
    }
    else if ( strncasecmp( text, "Host:", 5 ) == 0 )
    {
        text += 5;
        text += strspn( text, " \t" );
        host_ = text;
    }
    else
    {
        printf( "oop! unknow header %s\n", text );
    }

    return NO_REQUEST;

}
// 判断HTTP请求的消息体是否被完整的读入，无深入解析
httpConn::HTTP_CODE httpConn::parse_content( char* text )
{
    if ( read_index >= ( content_length + checked_index ) )
    {
        text[ content_length ] = '\0';
        return GET_REQUEST;
    }

    return NO_REQUEST;
}
// 解析HTTP请求的入口函数，主状态机
httpConn::HTTP_CODE httpConn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
   // 主状态机，从buffer中读取所有完整的行
    while ( ( ( check_state == CHECK_STATE_CONTENT ) && ( line_status == LINE_OK  ) )
                || ( ( line_status = parse_line() ) == LINE_OK ) )
    {
        text = get_line();
        start_line = checked_index;// 下一行的起始位置
        printf( "got 1 http line: %s\n", text );

        switch ( check_state )
        {
            case CHECK_STATE_REQUESTLINE:// 状态1，分析请求行
            {
                ret = parse_request_line( text );
                if ( ret == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:// 状态2，分析头部字段
            {
                ret = parse_headers( text );
                if ( ret == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                else if ( ret == GET_REQUEST )
                {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:// 状态3，分析请求消息体
            {
                ret = parse_content( text );
                if ( ret == GET_REQUEST )
                {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }

    return NO_REQUEST;
}
// 得到正确的HTTP请求后，分析目标文件的属性,若文件存在则使用mmap将其内存映射到file_address
httpConn::HTTP_CODE httpConn::do_request()
{
    strcpy( real_file, doc_root );
    int len = strlen( doc_root );
    strncpy( real_file + len, url_, FILENAME_LEN - len - 1 );
    if ( stat( real_file, &m_file_stat ) < 0 )
    {
        return NO_RESOURCE;
    }

    if ( ! ( m_file_stat.st_mode & S_IROTH ) )
    {
        return FORBIDDEN_REQUEST;
    }

    if ( S_ISDIR( m_file_stat.st_mode ) )
    {
        return BAD_REQUEST;
    }

    int fd = open( real_file, O_RDONLY );
    file_address = ( char* )mmap( 0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
}
// 对内存映射区执行munmao操作
void httpConn::unmap()
{
    if( file_address )
    {
        munmap( file_address, m_file_stat.st_size );
        file_address = 0;
    }
}
// 写HTTP响应
bool httpConn::write()
{
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = write_index;// 写缓冲区待发送的字节数
    if ( bytes_to_send == 0 )
    {
        modfd( epoll_fd, sock_fd, EPOLLIN );// set read event
        init();
        return true;
    }

    while( 1 )// ET 模式下的操作
    {
        temp = writev( sock_fd, iv_, iv__count );
        if ( temp <= -1 )
        {
            if( errno == EAGAIN )// 若TCP缓冲区没有空闲空间,则等到下一轮EPOLLOUT事件
            {
                modfd( epoll_fd, sock_fd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;
        // 发送响应成功
        if ( bytes_to_send <= bytes_have_send )
        {
            unmap();
            if( linger_ )
            {
                init();
                modfd( epoll_fd, sock_fd, EPOLLIN );
                return true;
            }
            else
            {
                modfd( epoll_fd, sock_fd, EPOLLIN );
                return false;
            } 
        }
    }
}
// 往写缓冲区写待发送数据
bool httpConn::add_response( const char* format, ... )
{
    if( write_index >= WRITE_BUFFER_SIZE )
    {
        return false;
    }
    va_list arg_list;
    va_start( arg_list, format );
    int len = vsnprintf( write_buf + write_index, WRITE_BUFFER_SIZE - 1 - write_index, format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - write_index ) )
    {
        return false;
    }
    write_index += len;
    va_end( arg_list );
    return true;
}

bool httpConn::add_status_line( int status, const char* title )
{
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool httpConn::add_headers( int content_len )
{
    add_content_length( content_len );
    add_linger();
    add_blank_line();
}

bool httpConn::add_content_length( int content_len )
{
    return add_response( "Content-Length: %d\r\n", content_len );
}

bool httpConn::add_linger()
{
    return add_response( "Connection: %s\r\n", ( linger_ == true ) ? "keep-alive" : "close" );
}

bool httpConn::add_blank_line()
{
    return add_response( "%s", "\r\n" );
}

bool httpConn::add_content( const char* content )
{
    return add_response( "%s", content );
}
// 根据处理结果，返回给客户端相应内容
bool httpConn::process_write( HTTP_CODE ret )
{
    switch ( ret )
    {
        case INTERNAL_ERROR:
        {
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if ( ! add_content( error_500_form ) )
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if ( ! add_content( error_400_form ) )
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
            add_status_line( 404, error_404_title );
            add_headers( strlen( error_404_form ) );
            if ( ! add_content( error_404_form ) )
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line( 403, error_403_title );
            add_headers( strlen( error_403_form ) );
            if ( ! add_content( error_403_form ) )
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line( 200, ok_200_title );
            if ( m_file_stat.st_size != 0 )
            {
                add_headers( m_file_stat.st_size );
                iv_[ 0 ].iov_base = write_buf;
                iv_[ 0 ].iov_len = write_index;
                iv_[ 1 ].iov_base = file_address;
                iv_[ 1 ].iov_len = m_file_stat.st_size;
                iv__count = 2;
                return true;
            }
            else
            {
                const char* ok_string = "<html><body></body></html>";
                add_headers( strlen( ok_string ) );
                if ( ! add_content( ok_string ) )
                {
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
    }

    iv_[ 0 ].iov_base = write_buf;
    iv_[ 0 ].iov_len = write_index;
    iv__count = 1;
    return true;
}
// 线程池中的工作线程调用,处理HTTP请求的入口函数
void httpConn::process()
{
    HTTP_CODE read_ret = process_read();
    if ( read_ret == NO_REQUEST )
    {
        modfd( epoll_fd, sock_fd, EPOLLIN );
        return;
    }

    bool write_ret = process_write( read_ret );
    if ( ! write_ret )
    {
        close_conn();
    }

    modfd( epoll_fd, sock_fd, EPOLLOUT );// add write event
}

}// namespace

