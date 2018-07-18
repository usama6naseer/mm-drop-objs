/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <limits>

#include "util.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"
#include "http_request.hh"
#include "http_response.hh"
#include "file_descriptor.hh"

#include <fstream>
#include <sstream>

using namespace std;

string safe_getenv( const string & key )
{
    const char * const value = getenv( key.c_str() );
    if ( not value ) {
        throw runtime_error( "missing environment variable: " + key );
    }
    return value;
}

/* does the actual HTTP header match this stored request? */
bool header_match( const string & env_var_name,
                   const string & header_name,
                   const HTTPRequest & saved_request )
{
    const char * const env_value = getenv( env_var_name.c_str() );

    /* case 1: neither header exists (OK) */
    if ( (not env_value) and (not saved_request.has_header( header_name )) ) {
        return true;
    }

    /* case 2: headers both exist (OK if values match) */
    if ( env_value and saved_request.has_header( header_name ) ) {
        return saved_request.get_header_value( header_name ) == string( env_value );
    }

    /* case 3: one exists but the other doesn't (failure) */
    return false;
}

string strip_query( const string & request_line )
{
    const auto index = request_line.find( "?" );
    if ( index == string::npos ) {
        return request_line;
    } else {
        return request_line.substr( 0, index );
    }
}

/* compare request_line and certain headers of incoming request and stored request */
unsigned int match_score( const MahimahiProtobufs::RequestResponse & saved_record,
                          const string & request_line,
                          const bool is_https )
{
    const HTTPRequest saved_request( saved_record.request() );

    /* match HTTP/HTTPS */
    if ( is_https and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTPS) ) {
        return 0;
    }

    if ( (not is_https) and (saved_record.scheme() != MahimahiProtobufs::RequestResponse_Scheme_HTTP) ) {
        return 0;
    }

    /* match host header */
    if ( not header_match( "HTTP_HOST", "Host", saved_request ) ) {
        return 0;
    }

    /* match user agent */
    if ( not header_match( "HTTP_USER_AGENT", "User-Agent", saved_request ) ) {
        return 0;
    }

    /* must match first line up to "?" at least */
    if ( strip_query( request_line ) != strip_query( saved_request.first_line() ) ) {
        return 0;
    }

    /* success! return size of common prefix */
    const auto max_match = min( request_line.size(), saved_request.first_line().size() );
    for ( unsigned int i = 0; i < max_match; i++ ) {
        if ( request_line.at( i ) != saved_request.first_line().at( i ) ) {
            return i;
        }
    }

    return max_match;
}

// trim from left
inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v")
{
    return ltrim(rtrim(s, t), t);
}

int main( void )
{
    try {
        assert_not_root();

        const string working_directory = safe_getenv( "MAHIMAHI_CHDIR" );
        const string recording_directory = safe_getenv( "MAHIMAHI_RECORD_PATH" );
        const string request_line = safe_getenv( "REQUEST_METHOD" )
            + " " + safe_getenv( "REQUEST_URI" )
            + " " + safe_getenv( "SERVER_PROTOCOL" );
        const bool is_https = getenv( "HTTPS" );

        SystemCall( "chdir", chdir( working_directory.c_str() ) );

        const vector< string > files = list_directory_contents( recording_directory );

        unsigned int best_score = 0;
        MahimahiProtobufs::RequestResponse best_match;

        for ( const auto & filename : files ) {
            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
            MahimahiProtobufs::RequestResponse current_record;
            if ( not current_record.ParseFromFileDescriptor( fd.fd_num() ) ) {
                throw runtime_error( filename + ": invalid HTTP request/response" );
            }

            unsigned int score = match_score( current_record, request_line, is_https );
            if ( score > best_score ) {
                best_match = current_record;
                best_score = score;
            }
        }

        const HTTPRequest temp_saved_request( best_match.request() );
        string request_line_to_save = temp_saved_request.first_line();

        string metadata_dir = "/home/usama/github/mahimahi-mod/metadata/";

        ifstream conf_file;
	    conf_file.open(metadata_dir + "conf.txt");
	    if (!conf_file) {
	        throw runtime_error("conf_file does not exist");
	    }

	    stringstream buffer;
	    buffer << conf_file.rdbuf();
	    string conf_str = buffer.str();
	    
	    int pos = conf_str.find(',');
	    string match_scheme = conf_str.substr(0, pos);
	    match_scheme = trim(match_scheme);
	    string request_to_match = conf_str.substr(pos+1, conf_str.size());
	    request_to_match = trim(request_to_match);

	    conf_file.close();


	    std::ofstream log_file;
		log_file.open (metadata_dir + "log/" + safe_getenv( "HTTP_HOST" ) + ".txt", std::ios_base::app);

	    bool drop_obj_flag = 0;
	    double match_percentage = 0.0;

	    if (match_scheme.compare("absolute") == 0) {
	     //    int match_len = 0;
	     //    const auto max_match = min( request_line.size(), request_to_match.size() );
		    // for ( unsigned int i = 0; i < max_match; i++ ) {
		    //     if ( request_line.at( i ) != request_to_match.at( i ) ) {
		    //         break;
		    //     }
		    //     match_len = i;
		    // }
	     //    match_percentage = match_len * 100 / max_match;
	     //    if (match_percentage >= 90) {
	     //    	drop_obj_flag = 1;
	     //    }

	    	std::ofstream temp_file;
			temp_file.open (metadata_dir + "log/temp.txt", std::ios_base::app);

	        unsigned int new_best_score = 0;
	        MahimahiProtobufs::RequestResponse new_best_match;
		    
		    for ( const auto & filename : files ) {
	            FileDescriptor fd( SystemCall( "open", open( filename.c_str(), O_RDONLY ) ) );
	            MahimahiProtobufs::RequestResponse new_current_record;
	            if ( not new_current_record.ParseFromFileDescriptor( fd.fd_num() ) ) {
	                throw runtime_error( filename + ": invalid HTTP request/response" );
	            }

	            unsigned int new_score = match_score( new_current_record, request_line, is_https );
	            if ( new_score > new_best_score ) {
	                new_best_match = new_current_record;
	                new_best_score = new_score;
	            }
	        }



	        const HTTPRequest saved_request( new_best_match.request() );
	        string new_request_to_match = saved_request.first_line();

            int match_len = 0;
	        const auto max_match = min( new_request_to_match.size(), request_to_match.size() );
		    for ( unsigned int i = 0; i < max_match; i++ ) {
		        if ( new_request_to_match.at( i ) != request_to_match.at( i ) ) {
		            break;
		        }
		        match_len = i;
		    }

		    temp_file << new_best_score << " " << new_request_to_match << " " << request_to_match << " " << request_line << " " << match_len << endl;

	        match_percentage = match_len * 100 / max_match;
	        if (match_percentage >= 90) {
	        	drop_obj_flag = 1;
	        }



	    }
	    else if (match_scheme.compare("substring") == 0) {
	        if (request_line.find(request_to_match) != std::string::npos) {
	        	drop_obj_flag = 1;
	        }	    	
	    }
	    else if (match_scheme.compare("dont") == 0) {
	    	// dont drop anything
	    }

        if ( best_score > 0 && drop_obj_flag == 0) { /* give client the best match */
            cout << HTTPResponse( best_match.response() ).str();

    		log_file << match_scheme << " | " << request_to_match << " | " << match_percentage << " | " << request_line << " | " << is_https << " | " << "not_dropped" << " | " << drop_obj_flag << " | " << request_line_to_save << "\n";
			log_file.close();

            return EXIT_SUCCESS;
        } else {                /* no acceptable matches for request */
            cout << "HTTP/1.1 404 Not Found" << CRLF;
            cout << "Content-Type: text/plain" << CRLF << CRLF;
            cout << "replayserver: could not find a match for " << request_line << CRLF;

            log_file << match_scheme << " | " << request_to_match << " | " << match_percentage << " | " << request_line << " | " << is_https << " | " << "dropped" << " | " << drop_obj_flag << " | " << request_line_to_save << "\n";
			log_file.close();

            return EXIT_FAILURE;
        }
    } catch ( const exception & e ) {
        cout << "HTTP/1.1 500 Internal Server Error" << CRLF;
        cout << "Content-Type: text/plain" << CRLF << CRLF;
        cout << "mahimahi mm-webreplay received an exception:" << CRLF << CRLF;
        print_exception( e, cout );
        return EXIT_FAILURE;
    }
}
