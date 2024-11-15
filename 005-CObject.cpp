/**
 * 创建JS类
 */
#include <quickjs.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

void fatal(const char* msg);
void js_dump_obj(JSContext* ctx, JSValue obj);
void js_dump_exception(JSContext* ctx);

typedef struct {
	int x;
	int y;
} JSPointData;

JSClassID js_point_class_id;

/**
 * @brief Point类的析构函数
 * 
 * @param rt	js引擎
 * @param val	待析构的Point对象
 */
void js_point_finalizer(JSRuntime *rt, JSValue val) {
	JSPointData *s=(JSPointData *)JS_GetOpaque(val, js_point_class_id);
	js_free_rt(rt, s);
}

/**
 * @brief Point类的构造函数
 * 
 * @param ctx			js语境
 * @param class_obj		Point类的class对象（保存有prototype等信息）
 * @param argc			构造函数传入的参数个数
 * @param argv			构造函数传入的各参数
 * 
 * @return 
 */
JSValue js_point_constructor(JSContext *ctx, JSValue class_obj, int argc, JSValue *argv) {
	JSPointData *data;
	JSValue obj = JS_UNDEFINED;
	
	if (argc!=2)
		return JS_ThrowReferenceError(ctx, "Need 2 arguments!");
	
	data = (JSPointData *)js_mallocz(ctx, sizeof(JSPointData));
	if (!data)
		return JS_EXCEPTION;
	
	if (JS_ToInt32(ctx, &data->x, argv[0]) != 0)
		goto fail;
	if (JS_ToInt32(ctx, &data->y, argv[1]) != 0) 
		goto fail;
	
	obj = JS_NewObjectClass(ctx, js_point_class_id);
	if (JS_IsException(obj))
		goto fail;
	JS_SetOpaque(obj, data);
	return obj;
fail:
	js_free(ctx, data);
	JS_FreeValue(ctx, obj);
	return JS_EXCEPTION;
}

/**
 * @brief Point对象的getx和gety方法
 * 
 * @param ctx 		js语境
 * @param this_val 	this对象
 * @param magic 	0表示getx, 1表示gety
 * 
 * @return 
 */
JSValue js_point_get_xy(JSContext *ctx, JSValue this_val, int magic) {
	JSPointData *data=(JSPointData *)JS_GetOpaque(this_val, js_point_class_id);
	if (data==NULL)
		return JS_EXCEPTION;
	switch(magic){
	case 0:
		return JS_NewInt32(ctx, data->x);
	case 1:
		return JS_NewInt32(ctx, data->y);
	default:
		return JS_ThrowRangeError(ctx, "Magic can't be %d!",magic);
	}
}

/**
 * @brief Point对象的setx和sety方法
 * 
 * @param ctx 		js语境
 * @param this_val	this对象
 * @param val 		要设置的值
 * @param magic 	0表示setx，1表示sety
 * 
 * @return 
 */
JSValue js_point_set_xy(JSContext* ctx, JSValue this_val, JSValue val, int magic) {
	JSPointData *data=(JSPointData *)JS_GetOpaque(this_val, js_point_class_id);
	int v;
	if (data==NULL)
		return JS_EXCEPTION;
	if (JS_ToInt32(ctx, &v, val)!=0)
		return JS_EXCEPTION;
	switch(magic) {
	case 0:
		data->x = v;
		break;
	case 1:
		data->y = v;
		break;
	default:
		return JS_ThrowRangeError(ctx, "Magic can't be %d!",magic);
	}
	return JS_UNDEFINED;
}

/**
 * @brief Point对象的norm方法
 * 
 * @param ctx 		js语境
 * @param this_val 	this对象
 * @param argc 		参数的个数
 * @param argv 		参数数组
 * 
 * @return 
 */
JSValue js_point_norm(JSContext* ctx, JSValue this_val, int argc, JSValue *argv) {
	JSPointData *data = (JSPointData *)JS_GetOpaque(this_val, js_point_class_id);
	if (data == NULL) 
		return JS_EXCEPTION;
	double x=data->x, y=data->y;
	return JS_NewFloat64(ctx, sqrt(x*x+y*y));
}

const JSClassDef js_point_class_def = {
	"Point",
	.finalizer = js_point_finalizer,
};

const JSCFunctionListEntry js_point_proto_funcs[]={
	JS_CGETSET_MAGIC_DEF("x", js_point_get_xy, js_point_set_xy, 0),  //0为magic值，在getxy和setxy中用其判断是处理x还是y
	JS_CGETSET_MAGIC_DEF("y", js_point_get_xy, js_point_set_xy, 1),  //1为magic值，在getxy和setxy中用其判断是处理x还是y
	JS_CFUNC_DEF("norm", 0, js_point_norm), // 0表示调用时需要的参数个数为0
};


int main() {
	//创建js引擎和语境对象
	JSRuntime *rt = JS_NewRuntime();
	if (rt==NULL)
		fatal("Can't create js runtime!");
	JS_SetDumpFlags(rt, 0x200 & 0x10000 );
	
	JSContext *ctx = JS_NewContext(rt);
	if (ctx==NULL)
		fatal("Can't create js context!");

	//注册Point类，js_point_class_id记录其id
	JS_NewClassID(rt, &js_point_class_id);
	JS_NewClass(rt, js_point_class_id, &js_point_class_def);
	
	//创建Point类的prototype对象
	JSValue point_proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, point_proto, js_point_proto_funcs, 
		sizeof(js_point_proto_funcs)/sizeof(js_point_proto_funcs[0]) );
	
	//创建Point类的构造函数
	JSValue point_ctor = JS_NewCFunction2(ctx, js_point_constructor, "Point", 2, JS_CFUNC_constructor, 0);
	
	//设置Point类的构造函数和prototype
	JS_SetConstructor(ctx, point_ctor, point_proto);
	JS_SetClassProto(ctx, js_point_class_id, point_proto);
	
	//将Point类的构造函数放入语境的全局对象作用域内
	JSValue global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx,global,"Point",point_ctor);
	
	//执行js脚本
	const char *foo_code = "var p=new Point(5,5); result=p.norm()+p.x+p.y; ";
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
	double result;
	JS_ToFloat64(ctx, &result, jsResult);
	printf("%lf\n", result);
	
	//清理
	JS_FreeValue(ctx, jsResult);
	JS_FreeValue(ctx, global);
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);	
	return 0;
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
