#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <dirent.h>


#include "types.h"
#include "utils.h"
#include "array.h"
#include "table.h"
#include "lexer.h"
#include "memory.h"
#include "object.h"
#include "data.h"

static char *STR_NULL = "NULL";
//static char *STR_CHAR = "CHAR";
//static char *STR_NUMBER = "NUMBER";
//static char *STR_DATA = "DATA";

value_t
data_sizeof(table_t *tbl)
{
    value_t res = 0;
    itable_t *t;
    for(t = tbl->begin; t != tbl->end; t = t->next){
        res += object_sizeof((object_t *) t->value);
    }
    return res;
}

table_t *
data_clone(table_t *tbl)
{
    table_t *res = table_create();
    itable_t *t;
    for(t = tbl->begin; t != tbl->end; t = t->next){
        object_t *obj;
        validate_format(!!(obj = object_clone((object_t *)t->value)),
			"data clone, object not clone");
        table_rpush(res, (value_p)obj);
    }
    return res;
}

table_t *
data_from(char *str)
{
    table_t *tbl = table_create();
    value_t i;
    for(i = 0; i < strlen(str); i++){
        object_t *obj;
        validate_format(!!(obj = object_define(TP_CHAR, sizeof(char_t))),
			"data from string, bad object malloc");
        *(char_t *)obj->ptr = (char_t)str[i];
        table_rpush(tbl, (value_p)obj);
    }
    return tbl;
}

table_t *
data_merge(table_t *target, table_t *source){
    itable_t *t;
    for(t = source->begin; t != source->end; t = t->next){
        table_rpush(target, t->value);
    }
    return target;
}

table_t *
data_format(table_t *tbl, table_t *format)
{
    table_t *deformed = table_create();
	itable_t *t = tbl->begin, *f;
	object_t *esp, *obj;

    while( t != tbl->end ){
        obj = (object_t *)t->value;

        if(obj->type == TP_NULL){
			value_t i;
			for(i = 0; i < strlen(STR_NULL); i++){
				validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
					"data from string, bad object malloc");
                *(char_t *)esp->ptr = (char_t)STR_NULL[i];
				table_rpush(deformed, (value_p)esp);
			}
			validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
				"data from string, bad object malloc");
			*(char_t *)esp->ptr = ' ';
			table_rpush(deformed, (value_p)esp);
            t = t->next;
            continue;
        }
		else
		if(obj->type == TP_CHAR){
			value_t num = 0;
			if(*(char_t *)obj->ptr == '%'){
				t = t->next;
				obj = (object_t *)t->value;

				if(*(char_t *)obj->ptr == 's'){
					validate_format(!!(f = table_rpop(format)), 
						"format, %%s require a string data!");
					
					obj = (object_t *)f->value;

					validate_format((obj->type == TP_ARRAY),
						"%%s must be input string data!");

                    data_merge(deformed, data_format((table_t *)obj->ptr, format));
					t = t->next;
					continue;
				}
				else
				if(*(char_t *)obj->ptr == '.'){
					t = t->next;
					obj = (object_t *)t->value;

					validate_format((obj->type == TP_CHAR),
						"object not a char type!");

					while(valid_digit(*(char_t *)obj->ptr)){
						validate_format(obj->type == TP_CHAR, 
							"object not a char type");
						num = num * 10 + *(char_t *)obj->ptr - '0';
						t = t->next;
						obj = (object_t *)t->value;
					}
				}

				if(*(char_t *)obj->ptr == 'n'){

					validate_format(!!(f = table_rpop(format)),
						"format, %%n require a number!");

					obj = (object_t *)f->value;

					char_t *fmt = qalam_malloc(sizeof(char_t) * 255);
					sprintf(fmt, "%%.%lldf", num);

					char_t *str_num = qalam_malloc(sizeof(char_t) * 255);
					if(((*(double64_t *)obj->ptr - *(long64_t *)obj->ptr) != 0) || (num > 0)){
						sprintf(str_num, fmt, *(double64_t *)obj->ptr);
					}
					else {
						sprintf(str_num, "%lld", *(long64_t *)obj->ptr);
					}

					qalam_free(fmt);

					value_t i;
					for(i = 0; i < strlen(str_num); i++){
						validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
							"data from string, bad object malloc");
						*(char_t *)esp->ptr = (char_t)str_num[i];
						table_rpush(deformed, (value_p)esp);
					}

					qalam_free(str_num);

					t = t->next;
					continue;
				}
				else
				if(*(char_t *)obj->ptr == 'h'){
					validate_format(!!(f = table_rpop(format)),
						"format, %%n require a number!");

					obj = (object_t *)f->value;

					char_t *fmt = qalam_malloc(sizeof(char_t) * 255);
					sprintf(fmt, "%%0%lldllx", num);

					char_t *str_num = qalam_malloc(sizeof(char_t) * 255);
					if(((*(double64_t *)obj->ptr - *(long64_t *)obj->ptr) != 0) || (num > 0)){
						sprintf(str_num, fmt, *(double64_t *)obj->ptr);
					}
					else {
						sprintf(str_num, "%llx", *(long64_t *)obj->ptr);
					}

					qalam_free(fmt);

					value_t i;
					for(i = 0; i < strlen(str_num); i++){
						validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
							"data from string, bad object malloc");
						*(char_t *)esp->ptr = (char_t)str_num[i];
						table_rpush(deformed, (value_p)esp);
					}

					qalam_free(str_num);

					t = t->next;
					continue;
				}
				else
				if(*(char_t *)obj->ptr == 'H'){
					if(!(f = table_rpop(format))){
						printf("format, %%n require a number!\n");
						exit(-1);
					}

					obj = (object_t *)f->value;

					char_t *fmt = qalam_malloc(sizeof(char_t) * 255);
					sprintf(fmt, "%%0%lldllX", num);

					char_t *str_num = qalam_malloc(sizeof(char_t) * 255);
					if(((*(double64_t *)obj->ptr - *(long64_t *)obj->ptr) != 0) || (num > 0)){
						sprintf(str_num, fmt, *(double64_t *)obj->ptr);
					}
					else {
						sprintf(str_num, "%llX", *(long64_t *)obj->ptr);
					}

					qalam_free(fmt);

					value_t i;
					for(i = 0; i < strlen(str_num); i++){
						validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
							"data from string, bad object malloc");
						*(char_t *)esp->ptr = (char)str_num[i];
						table_rpush(deformed, (value_p)esp);
					}

					qalam_free(str_num);

					t = t->next;
					continue;
				}

				validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
					"data from string, bad object malloc");

				*(char_t *)esp->ptr = *(char_t *)obj->ptr;

				table_rpush(deformed, (value_p)esp);

				t = t->next;
				continue;
			}

			validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
				"data from string, bad object malloc");

			*(char_t *)esp->ptr = *(char_t *)obj->ptr;

			table_rpush(deformed, (value_p)esp);
			t = t->next;
			continue;
		}
		else
		if(obj->type == TP_NUMBER){
			char *str_num = qalam_malloc(sizeof(char_t) * 255);
			if((*(double64_t *)obj->ptr - *(long64_t *)obj->ptr) != 0){
				sprintf(str_num, "%.16Lf", *(double64_t *)obj->ptr);
			}
			else {
				sprintf(str_num, "%lld", *(long64_t *)obj->ptr);
			}
			value_t i;
			for(i = 0; i < strlen(str_num); i++){
				validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
					"data from string, bad object malloc");
				*(char_t *)esp->ptr = (char_t)str_num[i];
				table_rpush(deformed, (value_p)esp);
			}

			qalam_free(str_num);

			validate_format(!!(esp = object_define(TP_CHAR, sizeof(char_t))),
				"data from string, bad object malloc");

			*(char_t *)esp->ptr = ' ';
			table_rpush(deformed, (value_p)esp);
			t = t->next;
			continue;
		}
		else
		if(obj->type == TP_ARRAY){
            data_merge(deformed, data_format((table_t *)obj->ptr, format));
			t = t->next;
			continue;
		}

		printf("this type is not acceptable!\n");
		exit(-1);
	}

	return deformed;
}

value_t
data_compare(table_t *tbl1, table_t *tbl2)
{
	if(table_count(tbl1) != table_count(tbl2)){
		return 0;
	}

	itable_t *t1 = tbl1->begin, *t2 = tbl2->begin;
	object_t *obj1, *obj2;

	while( t1 != tbl1->end && t2 != tbl2->end){
		obj1 = (object_t *)t1->value;
		obj2 = (object_t *)t2->value;
		if(obj1->type != obj2->type){
			return 0;
		}
		if(obj1->type == TP_NULL){
			if(obj1->ptr != obj2->ptr){
				return 0;
			}
		}
		else
		if(obj1->type == TP_NUMBER){
			if(*(double64_t *)obj1->ptr != *(double64_t *)obj2->ptr){
				return 0;
			}
		}
		else
		if(obj1->type == TP_CHAR){
			if(*(double64_t *)obj1->ptr != *(double64_t *)obj2->ptr){
				return 0;
			}
		}

		t1 = t1->next;
		t2 = t2->next;
	}

	return 1;
}

char *
data_to(table_t *tbl)
{
    char *str = qalam_malloc(table_count(tbl) * sizeof(char));
    value_t i = 0;
    itable_t *b;
    for(b = tbl->begin; b && (b != tbl->end); b = b->next){
        object_t *obj = (object_t *)b->value;
        if(obj->type == TP_CHAR){
            str[i++] = *(char_t *)obj->ptr;
            continue;
        }
    }
    return str;
}

void
data_delete(table_t *tbl)
{
    itable_t *b, *n;
    for( b = tbl->begin; b != tbl->end; b = n ){
        n = b->next;
        object_delete((object_t *)b->value);
        qalam_free(b);
    }
    qalam_free(tbl);
}


