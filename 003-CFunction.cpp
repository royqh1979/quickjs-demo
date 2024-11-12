/**
 * 使用C语言定义javascript函数，并在js脚本中调用
 */
#include <quickjs.h>
#include <stdio.h>
#include <stdint.h>

void fatal(const char* msg);
void js_dump_obj(JSContext* ctx, JSValue obj);
void js_dump_exception(JSContext* ctx);

//add函数定义
JSValue add(JSContext *ctx, JSValue this_val, int argc, JSValue* argv) {
	if (argc!=2) {
		return JS_ThrowRangeError(ctx,"Function add() needs exact 2 arguments!");
	}
	int32_t a,b;
	JS_ToInt32(ctx, &a, argv[0]);
	JS_ToInt32(ctx, &b, argv[1]);
	return JS_NewInt32(ctx, a+b);
}

int main() {
	//创建js引擎和语境对象
	JSRuntime *rt = JS_NewRuntime();
	if (rt==NULL)
		fatal("Can't create js runtime!");
	JSContext *ctx = JS_NewContext(rt);
	if (ctx==NULL)
		fatal("Can't create js context!");
	
	//创建add函数句柄
	JSValue addFunc = JS_NewCFunction(ctx,add,"add",2);
	//将add函数句柄设置成语境全局对象的add方法
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx,global,"add",addFunc);
	
	//执行js脚本
	const char *foo_code = "result = add(3,5);";
	JSValue evalResult = JS_Eval(ctx, foo_code, sizeof(foo_code), "<input>", JS_EVAL_FLAG_UNUSED);
	//如果执行不成功，显示错误信息
	if (JS_IsException(evalResult)) {
		js_dump_exception(ctx);
		JS_FreeContext(ctx);
		JS_FreeRuntime(rt);	
		fatal("Failed to eval foo()!");
	}

	//获取result变量的值
	JSValue jsResult = JS_GetPropertyStr(ctx, global, "result");
	int32_t result;
	JS_ToInt32(ctx, &result, jsResult);
	printf("Result: %d\n",result);
	
	//清理
	JS_FreeValue(ctx,jsResult);
	JS_FreeValue(ctx,global);
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);	
}

/**
 * @brief 显示错误提示并结束程序
 * 
 * @param msg 
 */
void fatal(const char* msg) {
	printf("Fatal: %s\n",msg);
	exit(-1);
}

/**
 * @brief 显示JS值内的字符串内容
 * 
 * @param ctx 
 * @param obj 
 */
void js_dump_obj(JSContext* ctx, JSValue obj) {
	const char *str = JS_ToCString(ctx, obj);
	if (str) {
		printf("%s\n", str);
		JS_FreeCString(ctx, str);	
	} else 
		printf("[Exception]\n");
}

/**
 * @brief 显示JS异常的具体内容
 * 
 * @param ctx 
 */
void js_dump_exception(JSContext* ctx) {
	JSValue exception = JS_GetException(ctx);
	js_dump_obj(ctx, exception);
	if (JS_IsError(ctx, exception)) {
		JSValue val = JS_GetPropertyStr(ctx, exception, "stack");
		if (!JS_IsUndefined(val)) {
			js_dump_obj(ctx, val);
		}
		JS_FreeValue(ctx, val);
	}	
}
