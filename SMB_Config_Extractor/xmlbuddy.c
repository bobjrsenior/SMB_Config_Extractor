#include "xmlbuddy.h"

#include <stdio.h>

static enum State {
	STATE_NEW,
	STATE_GENERAL,
	STATE_OPENING_TAG,
	STATE_ERROR,
	STATE_CLOSED
};

typedef struct XMLBuddy {
	FILE *output;
	enum State state;
	int prettyPrint;
	int indentation;
}XMLBuddy;

static int handleIndentation(XMLBuddy xmlBuddy) {
	// Stub for now
	return NO_ERROR;
}


XMLBuddy initXMLBuddy(char *filename, int prettyPrint) {
	XMLBuddy xmlBuddy = { NULL, STATE_NEW };
	FILE *output = fopen(filename, "w");
	if (output == NULL) {
		// What do?
		// Can't really return anything because XMLBuddy is a black box
		xmlBuddy.state = STATE_ERROR;
		return xmlBuddy;
	}
	xmlBuddy.output = output;
	xmlBuddy.state = STATE_GENERAL;
	xmlBuddy.prettyPrint = prettyPrint;
	xmlBuddy.indentation = 0;
	return xmlBuddy;
}

void close(XMLBuddy xmlBuddy) {
	fclose(xmlBuddy.output);
	xmlBuddy.output = NULL;
	xmlBuddy.state = STATE_CLOSED;
	return;
}

int startTagType(XMLBuddy xmlBuddy, enum tagType tag) {
	if (xmlBuddy.state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}
	handleIndentation(xmlBuddy);

}