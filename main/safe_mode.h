#ifndef _SAFE_MODE_H_
#define _SAFE_MODE_H_

extern PHPAPI int php_checkuid(const char *filename, int mode);
extern PHPAPI char *php_get_current_user(void);

#endif
