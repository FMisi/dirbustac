#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

#define MAX_LEN 256
#define MAX_THREADS 100

struct directory_data {
    char hostname[MAX_LEN];
    char directory[MAX_LEN];
};

void *worker(void *arg) {
    struct directory_data *data = (struct directory_data *)arg;

    CURL *curl;
    CURLcode res;
    char url[MAX_LEN];

    sprintf(url, "http://%s/%s", data->hostname, data->directory);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code == 200) {
                printf("Valid directory: %s\n", url);
                curl_easy_cleanup(curl);
                pthread_exit(NULL);
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <hostname> <directory_list>\n", argv[0]);
        return 1;
    }

    char hostname[MAX_LEN];
    strcpy(hostname, argv[1]);

    FILE *fp;
    char directory[MAX_LEN];

    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    while (fgets(directory, MAX_LEN, fp) != NULL) {
        directory[strcspn(directory, "\r\n")] = '\0';

        struct directory_data data;
        strcpy(data.hostname, hostname);
        strcpy(data.directory, directory);

        int res = pthread_create(&threads[thread_count++], NULL, worker, &data);
        if (res != 0) {
            perror("Error creating thread");
            return 1;
        }
    }

    fclose(fp);

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads, NULL);
    }

    curl_global_cleanup();

    return 0;
}
