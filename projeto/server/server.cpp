#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <cstring>
#include "../classes/User.h"

#define MAX_PENDING 5
#define MAX_LINE 256

#define ever (; ;)

using namespace std;

typedef struct _params {
    int id;
    int new_s;
    map<string, User> *users;
    list<Message> *messages;
    list<Message> *msg_feedback;


    pthread_mutex_t mutex;
} params_t;

void *spawn_thread(void *params);

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    unsigned int len;
    int s, new_s;
    int num_threads = 0;
    pthread_t *threads;
    params_t *params;
    int optval = 1;

    /* check parameter */
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(atoi(argv[1]));

    /* setup passive open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }

    /* configure the TCP socket to reuse address */
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("server: setsockopt");
        exit(1);
    }

    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }

    listen(s, MAX_PENDING);

    fprintf(stdout, "-------------------------\n");
    fprintf(stdout, "Server process ID : %d\n", getpid());
    fprintf(stdout, "-------------------------\n\n");

    /* users dictionary */
    map<string, User> users;
    /* messages list */
    list<Message> messages = {};
    list<Message> msg_feedback = {};


    /* wait for connection, then receive and print text */
    for ever {
        len = sizeof(sin);
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
            perror("simplex-talk: accept");
            exit(1);
        }

        params = (params_t *)malloc(++num_threads * sizeof(params_t));
        params[num_threads-1].id = num_threads;
        params[num_threads-1].new_s = new_s;
        params[num_threads-1].users = &users;
        params[num_threads-1].messages = &messages;
        params[num_threads-1].msg_feedback = &msg_feedback;
        pthread_mutex_init(&params[num_threads-1].mutex, NULL);
        threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
        pthread_create(&threads[num_threads], NULL, spawn_thread, &params[num_threads-1]);
    }

    return EXIT_SUCCESS;
}

void *spawn_thread(void *params) {
    params_t *pars = (params_t *)params;
    struct sockaddr_in peer;
    unsigned int peerlen;
    char buf[MAX_LINE];
    int status;
    User * usr;
    Message * msg;

    /* get client info */
    peerlen = sizeof(peer);
    if (getpeername(pars->new_s, (struct sockaddr *)&peer, &peerlen) < 0) {
        perror("simplex-talk: getpeername");
        exit(1);
    }

    /* get user's name */
    if(recv(pars->new_s, buf, sizeof(buf), 0) < 0) {
        perror("simplex-talk: recv");
        exit(1);
    }

    /* userid receives username */
    string userid(buf);
    buf[0] = '\0';

    /* if user's name is not in the dictionary, include it */
    if((*pars->users).find(userid) == (*pars->users).end()) {
        usr = new User(userid, Online, (*pars).new_s);
        (*pars->users).insert(make_pair(userid, *usr));
        usr = &((*pars->users).find(userid)->second);
        cout << "New user: " << userid << endl;
    }

    /* else set user status to online */
    else {

        /* get user object */
        usr = &((*pars->users).find(userid)->second);

        /* if the user is already online, issue a message */
        if(usr->isOnline()) {
            cout << "User " << userid << " is already logged!" << endl;

            if ((send(pars->new_s,
                    "Usuario ja conectado! Por favor, utilize outro usuario\n",
                    56, 0)) < 0) {
                perror("simplex-talk: send");
            }
        }
        else {
            usr->setM_status(Online);
            cout << "Welcome back, " << userid << endl;
        }
    }

    cout << "Number of registered users: " << (*pars->users).size() << endl;

    /* listen to the users command */
    while ((status = recv(pars->new_s, buf, sizeof(buf), 0))) {
        if(status < 0) {
            perror("simplex-talk: recv");
            continue;
        }
//        cout << "read thread " << pars->id << " socket " << pars->new_s << endl;

        /* evaluate command */
        string command(buf);
        buf[0] = '\0';
        stringstream output;

        vector<string> tokens;
        istringstream iss(command);
        copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

        // TODO verify malformed instructions
        /* WHO command */
        if(tokens[0] == "WHO") {
//            output << "From thread " << pars->id << endl;
            output << "| usuário  | status  |" << endl;

            /* get users and statuses */
            for(std::map<string, User >::iterator
                        it = (*pars->users).begin();
                it != (*pars->users).end(); it++) {
                output << "| " << (it->second).getM_name() << " | ";
                if((it->second).isOnline()) {
                    output << "online |" << endl;
                }
                else {
                    output << "offline |" << endl;
                }
            }

            strcpy(buf, output.str().c_str());
            /* DEBUG */
            fprintf(stdout,"output:\n%s\n", buf);
            if ((send(pars->new_s, buf, strlen(buf)+1, 0)) < 0) {
                perror("simplex-talk: send");
            }

        } else if (tokens[0] == "CREATEG") {
            /* CREATEG command */
            printf("CREATEG\n");

        } else if (tokens[0] == "JOING") {
            /* JOING command */
            printf("JOING\n");

        } else if (tokens[0] == "SENDG") {
            /* SENDG command */
            printf("SENDG\n");

        } else if (tokens[0] == "SEND") {
            /* SEND command */

            if( (*pars->users).find(tokens[1]) == (*pars->users).end() ){
                cout << "Receiver is not a registered user." << endl;
                continue;
            }

            string::size_type n = 0;

            /* erases first word (SEND) */
            n = command.find_first_not_of( " \t", n );
            n = command.find_first_of( " \t", n );
            command.erase( 0,  command.find_first_not_of( " \t", n ) );

//            /* erases second word (peer_name) */
            n = command.find_first_of( " \t", n );
            command.erase( 0,  command.find_first_not_of( " \t", n ) );
            /* now command only has the message */

            msg = new Message(command, usr, &((*pars->users).find(tokens[1])->second) );
            msg->setM_status(Queued);

            /* builds a string to be sent */
            stringstream msg_stream;
            string msg_sent;
            msg_stream << "Mensagem " << msg->getM_id() << " enfileirada." << "\n";
            msg_sent = msg_stream.str();
            char * cstr = new char [msg_sent.length()+1];
            strcpy (cstr, msg_sent.c_str());

            if ((send(pars->new_s, cstr, msg_sent.length()+1, 0)) < 0) {
                perror("simplex-talk: send");
            }

            (*pars->messages).push_back(*msg);

        } else if(tokens[0] == "EXIT") {

            /* EXIT command */
            usr->setM_status(Offline);
            if ((send(pars->new_s, "EXIT", 5, 0)) < 0) {
                perror("simplex-talk: send");
            }

            cout << "User " << usr->getM_name() << " logged out" << endl;
        }

        /* entrega de mensagens */
        for (auto it = (*pars->messages).begin(); it != (*pars->messages).end();) {
//            msg = &((*pars->messages).front());
//            cout << '[' << msg->getM_sender()->getM_name() << ">] ";
//            cout << msg->getM_message() << '\n';

            if( !(it->getM_receiver()->isOnline()) )
                continue;

            stringstream send_msg_stream;
            string send_msg;
            send_msg_stream << "[" << it->getM_sender()->getM_name() << "] ";
            send_msg_stream << it->getM_message();
            send_msg = send_msg_stream.str();

            if ((send( it->getM_receiver()->getM_socket(), send_msg.c_str(), send_msg.length()+1, 0)) < 0) {
                perror("simplex-talk: send");
            }

            it->setM_status(Delivered);
            (*pars->msg_feedback).push_back(*it);

            it = (*pars->messages).erase(it);
        }

        /* entrega de avisos */
        for (auto it = (*pars->msg_feedback).begin(); it != (*pars->msg_feedback).end();) {

            if( !(it->getM_sender()->isOnline()) ){
                continue;
            }

            /* builds a string to be sent */
            stringstream msg_stream;
            string msg_sent;
            msg_stream << "Mensagem " << it->getM_id() << " entregue." << "\n";
            msg_sent = msg_stream.str();
//            char * cstr = new char [msg_sent.length()+1];
//            strcpy (cstr, msg_sent.c_str());

//            cout << "COCOA: " << cstr << msg_sent.length()+1 << "KD" << endl;

            if ((send( it->getM_sender()->getM_socket(), msg_sent.c_str(), msg_sent.length()+1, 0)) < 0) {
                perror("simplex-talk: send");
            }

            it = (*pars->msg_feedback).erase(it);
        }
    }

    return NULL;
}
