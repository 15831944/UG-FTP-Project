#ifndef _REPLYCODE_H_
#define _REPLYCODE_H_

typedef int ReplyCode;
#define genCode(a, b, c) ((ReplyCode)((a - '0') * 100 + (b - '0') * 10 + c - '0'))
struct StdReply{
	const static ReplyCode SERVER_READY = 220;
	const static ReplyCode RIGHT_USERNAME = 331;
	const static ReplyCode LOGIN_SUCCESS = 230;
	const static ReplyCode ENTER_PASSIVE = 227;
	const static ReplyCode TRANSFERING = 125;
	const static ReplyCode OPENING_DATAPORT = 150;
};

#endif /* _REPLYCODE_H_ */
