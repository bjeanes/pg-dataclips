#include <curl/curl.h>
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

struct CURLBuffer {
  char *memory;
  size_t size;
};

void abort_if_preconditions_not_met(ReturnSetInfo* rsinfo);
Datum dataclip(PG_FUNCTION_ARGS);
struct CURLBuffer* _curl_get_file_contents_by_url(char* url);
static size_t _curl_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

PG_FUNCTION_INFO_V1(dataclip);
  Datum
dataclip(PG_FUNCTION_ARGS)
{
  FuncCallContext* funcctx;
  TupleDesc        tupdesc;

  if(SRF_IS_FIRSTCALL())
  {
    abort_if_preconditions_not_met((ReturnSetInfo*) fcinfo->resultinfo);

    MemoryContext oldcontext;

    funcctx = SRF_FIRSTCALL_INIT();
    oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

    /*
       - fetch dataclip CSV/JSON
       - parse out fields (and types if possible?)
       - build TupleDesc for record
       - set `funcctx->max_calls` to # of records in CSV/JSON
       - store CSV/JSON (as string?) in multi_call_memory_ctx
       */

    /* temporary c-ext learning code: */

    /* Build a tuple descriptor for our result type */
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
      ereport(ERROR,
          (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
           errmsg("function returning record called in context "
             "that cannot accept type record")));

    /*
     * generate attribute metadata needed later to produce tuples from raw
     * C strings
     */
    funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);
    funcctx->max_calls = 3; // will come from # of rows in JSON/CSV

    /* end learning code */

    MemoryContextSwitchTo(oldcontext);
  }

  funcctx = SRF_PERCALL_SETUP();

  if (funcctx->call_cntr < funcctx->max_calls)
  {
    /*
       - parse `funcctx->call_cntr`th row from stored CSV/JSON
       - create datum with row/record (BuildTupleFromCStrings ?)
       - return as next result
       */
    Datum result = (Datum) 0;
    SRF_RETURN_NEXT(funcctx, result);
  }
  else SRF_RETURN_DONE(funcctx);
}

static size_t
_curl_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct CURLBuffer *mem = (struct CURLBuffer*)userp;

  mem->memory = repalloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

struct CURLBuffer *
_curl_get_file_contents_by_url(char* url)
{
  CURL *curl_handle;
  CURLcode res;

  struct CURLBuffer* chunk = palloc0(sizeof(struct CURLBuffer));

  chunk->memory = palloc(1);
  chunk->size   = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _curl_memory_callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 3);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "pgdataclips/0.0.1"); // FIXME: pull version in at compilet time

  res = curl_easy_perform(curl_handle);

  if(res != CURLE_OK) {
    return NULL;
  }

  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();

  return chunk;
}

void
abort_if_preconditions_not_met(ReturnSetInfo* rsinfo)
{
  if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
  {
    ereport(ERROR,
        (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
         errmsg("set-valued function called in context that cannot accept a set")));
  }

  if (!(rsinfo->allowedModes & SFRM_Materialize) ||
      rsinfo->expectedDesc == NULL)
  {
    ereport(ERROR,
        (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
         errmsg("materialize mode required, but it is not " \
           "allowed in this context")));
  }
}
