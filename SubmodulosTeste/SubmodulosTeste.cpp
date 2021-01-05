
#define CURL_STATICLIB

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

/* silly test data to POST */
static const char data[] = "<Request><Login>my_login</Login><Password>my_password</Password></Request>";
struct WriteThis {
    const char* readptr;
    size_t sizeleft;
};

static size_t read_callback(void* dest, size_t size, size_t nmemb, void* userp)
{
    struct WriteThis* wt = (struct WriteThis*)userp;
    size_t buffer_size = size * nmemb;

    if (wt->sizeleft) {
        /* copy as much as possible from the source to the destination */
        size_t copy_this_much = wt->sizeleft;
        if (copy_this_much > buffer_size)
            copy_this_much = buffer_size;
        memcpy(dest, wt->readptr, copy_this_much);

        wt->readptr += copy_this_much;
        wt->sizeleft -= copy_this_much;
        return copy_this_much; /* we copied this many bytes */
    }

    return 0; /* no more data left to deliver */
}

int main(void)
{
    CURL* curl;
    CURLcode res;

    struct WriteThis wt;

    wt.readptr = data;
    wt.sizeleft = strlen(data);

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* Check for errors */
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n",
            curl_easy_strerror(res));
        return 1;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/xml");

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
        /* URL sandbox para testes: */
        curl_easy_setopt(curl, CURLOPT_URL, "https://reqbin.com/echo/post/xml");
        /*URL real para transações reais: https://e-commerce.stone.com.br*/

        /* Now specify we want to POST data */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        /*Colocar cabeçalhos */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        /* pointer to pass to our read function */
        curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

        /* get verbose debug output please */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        /*
          If you use POST to a HTTP 1.1 server, you can send data without knowing
          the size before starting the POST if you use chunked encoding. You
          enable this by adding a header like "Transfer-Encoding: chunked" with
          CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
          specify the size in the request.
        */
#ifdef USE_CHUNKED
        {
            struct curl_slist* chunk = NULL;

            chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
               list again */
        }
#else
        /* Set the expected POST size. If you want to POST large amounts of data,
           consider CURLOPT_POSTFIELDSIZE_LARGE */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);
#endif

#ifdef DISABLE_EXPECT
        /*
          Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
          header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
          NOTE: if you want chunked transfer too, you need to combine these two
          since you can only set one list of headers with CURLOPT_HTTPHEADER. */

          /* A less good option would be to enforce HTTP 1.0, but that might also
             have other implications. */
        {
            struct curl_slist* chunk = NULL;

            chunk = curl_slist_append(chunk, "Expect:");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
               list again */
        }
#endif

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}