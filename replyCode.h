#ifndef _REPLYCODE_H_
#define _REPLYCODE_H_

typedef int ReplyCode;
#define genCode(a, b, c) ((ReplyCode)((a - '0') * 100 + (b - '0') * 10 + c - '0'))
struct StdReply{
	const static ReplyCode TRANSFERING = 125;
	const static ReplyCode OPENING_DATAPORT = 150;
	const static ReplyCode NAME_SYSTEM_TYPE = 215;
	const static ReplyCode SERVER_READY = 220;
	const static ReplyCode ENTER_PASSIVE = 227;
	const static ReplyCode LOGIN_SUCCESS = 230;
	const static ReplyCode RIGHT_USERNAME = 331;
	const static ReplyCode USERNAME_FIRST = 503;
	const static ReplyCode COMMAND_NOT_IMPLEMENTED = 502;
	const static ReplyCode LOGIN_INCORRECT = 530;
};

enum LoginStatus{ LOGIN_INIT, LOGIN_INPUT_USERNAME, LOGIN_LOGINED};

#endif /* _REPLYCODE_H_ */
