#include "agent-c.h"

static void json_escape(const char* input, char* output, size_t size) {
    const char* s = input;
    char* d = output;
    while (*s && (size_t)(d - output) < size - 4) {
        if (*s == '"') { *d++ = '\\'; *d++ = '"'; }
        else if (*s == '\\') { *d++ = '\\'; *d++ = '\\'; }
        else if (*s == '\n') { *d++ = '\\'; *d++ = 'n'; }
        else if (*s == '\r') { *d++ = '\\'; *d++ = 'r'; }
        else if (*s == '\t') { *d++ = '\\'; *d++ = 't'; }
        else { *d++ = *s; }
        s++;
    }
    *d = '\0';
}

static char* json_find(const char* json, const char* key, char* out, size_t size) {
    if (!json || !key || !out) return NULL;
    char pattern[64];
    snprintf(pattern, 64, "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return NULL;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == '\t' || *p == ':' || *p == '{')) p++;

    if (*p == '"') {
        p++;
        char* o = out;
        while (*p && *p != '"' && (size_t)(o - out) < size - 1) {
            if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == 'n')) {
                if (p[1] == 'n') *o++ = '\n';
                else *o++ = p[1];
                p += 2;
            } else {
                *o++ = *p++;
            }
        }
        *o = '\0';
    } else {
        const char* end = p;
        while (*end && *end != ',' && *end != '}' && *end != ']') end++;
        size_t len = end - p;
        if (len >= size) len = size - 1;
        strncpy(out, p, len);
        out[len] = '\0';
    }
    return out;
}

char* json_request(const Agent* agent, const Config* config, char* out, size_t size) {
    char messages_json[MAX_BUFFER] = "[";
    for (int i = 0; i < agent->msg_count; i++) {
        if (i > 0) strcat(messages_json, ",");
        char escaped[MAX_CONTENT * 2];
        json_escape(agent->messages[i].content, escaped, sizeof(escaped));
        char temp[MAX_CONTENT * 2 + 64];
        snprintf(temp, sizeof(temp), "{\"role\":\"%s\",\"content\":\"%s\"}",
                 agent->messages[i].role, escaped);
        if (strlen(messages_json) + strlen(temp) < MAX_BUFFER - 10) strcat(messages_json, temp);
    }
    strcat(messages_json, "]");

    const char* template = "{\"model\":\"%s\",\"messages\":%s,\"temperature\":%.1f,\"max_tokens\":%d,"
                           "\"tool_choice\":\"auto\",\"tools\":[{\"type\":\"function\",\"function\":{"
                           "\"name\":\"execute_command\",\"description\":\"Execute shell command\","
                           "\"parameters\":{\"type\":\"object\",\"properties\":{\"command\":{\"type\":\"string\"}},"
                           "\"required\":[\"command\"]}}}]}";
    snprintf(out, size, template, config->model, messages_json, config->temp, config->max_tokens);
    return out;
}

char* json_content(const char* response, char* out, size_t size) {
    return json_find(response, "content", out, size);
}

int extract_command(const char* response, char* cmd, size_t cmd_size) {
    char args_buf[2048];
    if (!json_find(response, "arguments", args_buf, sizeof(args_buf))) return 0;
    return json_find(args_buf, "command", cmd, cmd_size) != NULL;
}
