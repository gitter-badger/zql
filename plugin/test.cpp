#include <stdio.h>
#include <string.h>
#include <zmq.h>
#include "Request.h"
#include "ObjectParser.h"

void *context;
void *socket;

class InsertListener : public ObjectListener {

public:
    virtual void OnRequestGet(unsigned int requestId, RequestGet *request) {
        printf("request get\n");
    }

    virtual void OnResponseGetEmpty(unsigned int requestId) {
        printf("response get empty\n");
    }

    virtual void OnResponseGetOk(unsigned int requestId, ResponseGetOk *response) {
        printf("response get ok\n");
    }

    virtual void OnRequestAdd(unsigned int requestId, RequestAdd *request) {
        printf("request add\n");
    }


    virtual void OnResponseAddOk(unsigned int requestId) {
        printf("response add ok: %u\n", requestId);
    }

    virtual void OnResponseAddTraverse(unsigned int requestId, ResponseAddTraverse *response) {
        printf("response add traverse, request id %u init request id %u, ciphertext size %u\n", requestId, response->initRequestId(), response->ciphertext()->size());

        RequestAddContinue request;
        request.initRequestId() = response->initRequestId();
        request.compareResult() = 1;

        RequestWrapper wrapper;
        wrapper.setId((unsigned int) rand());
        wrapper.setRequest(&request);

        CborOutput output(10000);
        CborWriter writer(output);
        wrapper.Serialize(writer);

        zmq_send(socket, output.getData(), (size_t) output.getSize(), 0);

        zmq_msg_t message;
        zmq_msg_init(&message);
        zmq_msg_recv(&message, socket, 0);

        CborInput input(zmq_msg_data(&message), zmq_msg_size(&message));
        ObjectParser parser;
        parser.SetInput(input);
        parser.SetListener(*this);
        parser.Run();

        delete response;
    }


    virtual void OnRequestAddContinue(unsigned int requestId, RequestAddContinue *request) {
        printf("request add continue\n");
    }

    virtual void OnError(const char *error) {
        printf("error: %s\n", error);
    }
};

int main(int argc, char **argv) {

	context = zmq_ctx_new();
	socket = zmq_socket(context, ZMQ_REQ);

	zmq_connect(socket, "tcp://127.0.0.1:9990");

    /*
    RequestGet request;
    RequestWrapper wrapper;

    request.database() = "testdatabase";
    request.table() = "testtable";
    request.pk() = 3;
    */



    RequestAdd request;
    request.database() = argv[1];
    request.table() = argv[2];
    request.row()[argv[3]] = argv[4];
    request.row()[argv[5]] = argv[6];


    RequestWrapper wrapper;
    wrapper.setId(123);
    wrapper.setRequest(&request);

    CborOutput output(10000);
    CborWriter writer(output);
    wrapper.Serialize(writer);

    zmq_send(socket, output.getData(), output.getSize(), 0);

    zmq_msg_t message;
    zmq_msg_init(&message);
    zmq_msg_recv(&message, socket, 0);

    CborInput input(zmq_msg_data(&message), zmq_msg_size(&message));
    ObjectParser parser;
    InsertListener listener;
    parser.SetInput(input);
    parser.SetListener(listener);
    parser.Run();

    /*
	//	for(int i = 0; i < 1000000; i++) {
		zmq_send(socket, data, size, 0);
	
		zmq_msg_t message;
		zmq_msg_init(&message);
		zmq_msg_recv(&message, socket, 0);
		
		//		printf("request %d response data: %.*s\n", i, zmq_msg_size(&message), zmq_msg_data(&message));

		zmq_msg_close(&message);

		//if(i % 10000 == 0) {
		//	printf("request %d\n", i);
		//}

		//}
*/
	zmq_close(socket);
	zmq_ctx_destroy(context);
	return 0;
}

