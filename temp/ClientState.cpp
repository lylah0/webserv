#include "ClientState.hpp"

ClientState::ClientState()
{
	this->headersComplete = false;
	this->requestReady = false;
	this->headerEnd = 0;
	this->contentLength = 0;
	this->keepAlive = false;
}
