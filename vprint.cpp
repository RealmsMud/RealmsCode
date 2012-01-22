 /*
 * vprint.cpp
 *	 Functions related to vprint (printf replacement for realms)
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "login.h"
#include "vprint.h"
#include <stdarg.h>

// Function Prototypes
bstring delimit(const char *str, int wrap);

void Creature::bPrint(bstring toPrint) const {
	printColor("%s", toPrint.c_str());
}

void Creature::print(const char *fmt,...) const {
	// Mad hack, but it'll stop some stupid errors
	if(!this)
		return;

	Socket* printTo = getSock();

	if(isPet())
		printTo = getConstMaster()->getSock();

	if(!this || !printTo)
		return;

	va_list ap;

	va_start(ap, fmt);
	if(isPet()) {
		printTo->print("Pet> ");
	}
	printTo->vprint(fmt, ap);
	va_end(ap);
}

void Creature::printColor(const char *fmt,...) const {
	Socket* printTo = getSock();

	if(isPet())
		printTo = getConstMaster()->getSock();

	if(!this || !printTo)
		return;

	va_list ap;

	va_start(ap, fmt);
	if(isPet()) {
		printTo->print("Pet> ");
	}
	printTo->vprint(fmt, ap);
	va_end(ap);
}
void Player::vprint(const char *fmt, va_list ap) const {
	if(this && mySock)
		mySock->vprint(fmt, ap);
}


#ifndef __CYGWIN__
// vprint, called by print and other variable argument print functions
// absolutely not thread safe ;)

static int VPRINT_flags = 0;

void Socket::vprint(const char *fmt, va_list ap) {
	char	*msg;
	va_list	aq;

	if(!this) {
		std::cout << "vprint(): called with null this! :(\n";
		return;
	}

	VPRINT_flags = 0;
	if(myPlayer)
		VPRINT_flags = myPlayer->displayFlags();
	// Incase vprint is called multiple times with the same ap
	// (in which case ap would be undefined, so make a copy of it
	va_copy(aq, ap);
	int n = vasprintf(&msg, fmt, aq);
	va_end(aq);

	if(n == -1) {
		std::cout << "Problem with vasprintf in vprint!" << std::endl;
		return;
	}
	bstring toPrint;

    if(!myPlayer || (myPlayer && myPlayer->getWrap() == -1))
        toPrint = delimit( msg, getTermCols() - 4);
    else if(myPlayer && myPlayer->getWrap() > 0)
        toPrint = delimit( msg, myPlayer->getWrap());
    else
        toPrint = msg;
    bprint(toPrint + "^x");

	free(msg);
}

int print_objcrt(FILE *stream, const struct printf_info *info, const void *const *args) {
	char *buffer;
	int len;

	if(info->spec == 'B') {
		const bstring *tmp = *((const bstring **) (args[0]));
		len = asprintf(&buffer, "%s", tmp->c_str());
	}
	else if(info->spec == 'T') {
		const std::ostringstream *tmp = *((const std::ostringstream **) (args[0]));
		len = asprintf(&buffer, "%s", tmp->str().c_str());
	}
	// M = Capital Monster N = small monster
	else if(info->spec == 'M' || info->spec == 'N') {
		const Creature *crt = *((const Creature **) (args[0]));
		if(info->spec == 'M')
			len = asprintf(&buffer, "%s", crt_str(crt, info->width, VPRINT_flags | CAP));
		else
			len = asprintf(&buffer, "%s", crt_str(crt, info->width, VPRINT_flags));
		if(len == -1)
			return(-1);
	}
	else if(info->spec == 'R') {
		const Creature *crt = *((const Creature **) (args[0]));
		len = asprintf(&buffer, "%s", crt->name);
		if(len == -1)
			return(-1);
	}
	// O = Capital Object P = small object
	else if(info->spec == 'O' || info->spec == 'P') {
		const Object *obj = *((const Object **) (args[0]));
		if(info->spec == 'O')
			len = asprintf(&buffer, "%s", obj->getObjStr(NULL, VPRINT_flags | CAP, info->width).c_str());
		else
			len = asprintf(&buffer, "%s", obj->getObjStr(NULL, VPRINT_flags, info->width).c_str());

		if(len == -1)
			return(-1);
	}
	// Unhandled type
	else {
		return(-1);
	}

	// Pad to the minimum field width and print to the stream.
	//len = fprintf (stream, "%*s", (info->left ? -info->width : info->width), buffer);
	len = fprintf(stream, "%s", buffer);

	// Clean up and return.
	free(buffer);
	return(len);
}

int print_arginfo (const struct printf_info *info, size_t n, int *argtypes) {
	// We always take exactly one argument and this is a pointer to the structure..
	if(n > 0)
		argtypes[0] = PA_POINTER;
	return(1);
}

int Server::installPrintfHandlers() {
	int r = 0;
	// bstring
	r |= register_printf_function('B', print_objcrt, print_arginfo);
	// std::ostringstream
	r |= register_printf_function('T', print_objcrt, print_arginfo);
	// creature
	r |= register_printf_function('N', print_objcrt, print_arginfo);
	// capital creature
	r |= register_printf_function('M', print_objcrt, print_arginfo);
	// object
	r |= register_printf_function('P', print_objcrt, print_arginfo);
	// capital object
	r |= register_printf_function('O', print_objcrt, print_arginfo);
	// creature's real name
	r |= register_printf_function('R', print_objcrt, print_arginfo);
	return(r);
}

#else


char *obj_str(const Object *obj, int num, int flag );

// vprint, only used on cygwin
//
void Socket::vprint(const char *fmt, va_list ap) {
	char	msg[8192];
	char	*fmt2;
	int		i = 0, j = 0, k;
	int		num, loc, ind = -1, len, flags = 0;
	int		arg[14];
	char	type;


	if(myPlayer)
		flags = myPlayer->displayFlags();

	len = strlen(fmt);
	fmt2 = new char[len+1];
	if(!fmt2)
		merror("print", FATAL);

	arg[0] = va_arg(ap, int);
	arg[1] = va_arg(ap, int);
	arg[2] = va_arg(ap, int);
	arg[3] = va_arg(ap, int);
	arg[4] = va_arg(ap, int);
	arg[5] = va_arg(ap, int);
	arg[6] = va_arg(ap, int);
	arg[7] = va_arg(ap, int);
	arg[8] = va_arg(ap, int);
	arg[9] = va_arg(ap, int);
	arg[10] = va_arg(ap, int);
	arg[11] = va_arg(ap, int);
	arg[12] = va_arg(ap, int);
	arg[13] = va_arg(ap, int);


	// Check for special handlers and modify arguments as necessary
	do {
		if(fmt[i] == '%') {
			fmt2[j++] = fmt[i];
			num = 0;
			k = i;
			do {
				k++;
				if(	(fmt[k] >= 'a' && fmt[k] <= 'z') ||
					(fmt[k] >= 'A' && fmt[k] <= 'Z') ||
					fmt[k] == '%'
				) {
					loc = k;
					type = fmt[k];
					break;
				} else if(fmt[k] >= '0' && fmt[k] <= '9')
					num = num * 10 + fmt[k] - '0';
			} while(k < len);

			if(type == '%') {
				fmt2[j++] = '%';
				i++;
				i++;
				continue;
			}

			ind++;
			if(	type != 'N' &&	// creature
				type != 'M' &&	// capital creature
				type != 'O' &&	// capital object
				type != 'P' &&	// object
				type != 'R' &&	// creature's real name
				type != 'B' &&	// bstring
				type != 'T'		// ostringstream
			) {
				i++;
				continue;
			}

			i = loc + 1;
			fmt2[j++] = 's';

			switch (type) {
			case 'B':
				arg[ind] = (int) ((bstring *) arg[ind])->c_str();
				continue;
			case 'T':
				arg[ind] = (int) ((std::ostringstream *) arg[ind])->str().c_str();
				continue;
			case 'N':
				arg[ind] = (int) crt_str((Creature *) arg[ind], num, flags);
				continue;
			case 'M':
				arg[ind] = (int) crt_str((Creature *) arg[ind], num, flags | CAP);
				continue;
			case 'P':
				arg[ind] = (int) obj_str((Object *) arg[ind], num, flags);
				continue;
			case 'O':
				arg[ind] = (int) obj_str((Object *) arg[ind], num, flags | CAP);
				continue;
			case 'R':
				arg[ind] = (int) (((Creature *)arg[ind])->name);
				continue;
			}
		}
		fmt2[j++] = fmt[i++];
	} while(i < len);

	fmt2[j] = 0;
	snprintf(msg, 8000, fmt2, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14]);
	delete fmt2;


	bstring toPrint;

    toPrint = delimit( msg, getTermCols() - 4);
    bprint(toPrint);
}


#endif

