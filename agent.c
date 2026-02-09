#include "agent-c.h"
#include <sys/wait.h>

extern Agent agent;
extern Config config;

void init_agent(void) {
    strcpy(agent.messages[0].role, "system");
    strcpy(agent.messages[0].content, "你是一个有用的AI助手。请始终用中文回复用户。"
           "如果用户要求操作文件或执行命令，你必须使用 execute_command 工具来完成。"
           "\n\n重要规则：每次工具执行完成后，你必须：\n"
           "1. 详细解释刚才执行的命令是什么意思\n"
           "2. 逐个解释命令中的每个参数和选项的作用\n"
           "3. 说明命令的执行结果\n"
           "例如：'ls -la' 命令用于列出目录内容，其中 -l 表示使用长格式显示详细信息，-a 表示显示所有文件包括隐藏文件。");
    agent.msg_count = 1;
}

int execute_command(const char* response) {
    char cmd[1024] = {0};
    if (!extract_command(response, cmd, sizeof(cmd)) || !*cmd) {
        return 0;
    }

    printf("\n\033[1;33m[命令] %s\033[0m\n", cmd);
    printf("是否执行此命令? [y/N]: ");
    fflush(stdout);

    char answer[10];
    if (!fgets(answer, sizeof(answer), stdin)) {
        printf("已跳过。\n");
        return 0;
    }

    if (answer[0] != 'y' && answer[0] != 'Y') {
        printf("命令执行已取消。\n");
        if (agent.msg_count < MAX_MESSAGES - 1) {
            strcpy(agent.messages[agent.msg_count].role, "tool");
            strcpy(agent.messages[agent.msg_count].content, "用户取消了命令执行。");
            agent.msg_count++;
        }
        return 0;
    }

    printf("\033[1;32m[执行中...]\033[0m\n");

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        printf("\033[1;31m[错误] 命令执行失败\033[0m\n");
        if (agent.msg_count < MAX_MESSAGES - 1) {
            strcpy(agent.messages[agent.msg_count].role, "tool");
            strcpy(agent.messages[agent.msg_count].content, "命令执行失败。");
            agent.msg_count++;
        }
        return 0;
    }

    char result[MAX_CONTENT] = {0};
    size_t n = fread(result, 1, sizeof(result)-1, pipe);
    result[n] = '\0';
    int exit_code = pclose(pipe);

    if (n > 0) printf("%s", result);

    // 检查命令执行状态
    if (exit_code == 0) {
        printf("\033[1;32m✓ 命令执行成功\033[0m\n");
    } else {
        printf("\033[1;31m✗ 命令执行失败，退出码: %d\033[0m\n", WEXITSTATUS(exit_code));
    }

    if (agent.msg_count < MAX_MESSAGES - 1) {
        strcpy(agent.messages[agent.msg_count].role, "tool");
        if (exit_code == 0) {
            strncpy(agent.messages[agent.msg_count].content, n > 0 ? result : "命令执行成功。", MAX_CONTENT-1);
        } else {
            char error_msg[MAX_CONTENT];
            snprintf(error_msg, MAX_CONTENT, "命令执行失败（退出码 %d）。输出: %s", 
                     WEXITSTATUS(exit_code), n > 0 ? result : "(无输出)");
            strncpy(agent.messages[agent.msg_count].content, error_msg, MAX_CONTENT-1);
        }
        agent.msg_count++;
    }
    return 1;
}

int process_agent(const char* task) {
    if (agent.msg_count >= MAX_MESSAGES - 2) agent.msg_count = 1;

    strcpy(agent.messages[agent.msg_count].role, "user");
    strncpy(agent.messages[agent.msg_count].content, task, MAX_CONTENT-1);
    agent.msg_count++;

    char req[MAX_BUFFER], resp[MAX_BUFFER], content[MAX_CONTENT];
    
    json_request(&agent, &config, req, sizeof(req));
    if (http_request(req, resp, sizeof(resp)) != 0) return -1;

    // 如果检测到工具调用
    if (strstr(resp, "\"tool_calls\"")) {
        if (execute_command(resp)) {
            // 执行后再次请求 API 以获取 AI 的回应
            printf("\033[90m[调试] 正在请求AI解释命令...\033[0m\n");
            json_request(&agent, &config, req, sizeof(req));
            if (http_request(req, resp, sizeof(resp)) != 0) {
                printf("\033[31m[错误] 第二次API调用失败\033[0m\n");
                return -1;
            }
        }
    }

    // 提取并显示AI的回复
    content[0] = '\0';
    if (json_content(resp, content, sizeof(content))) {
        if (strlen(content) > 0) {
            printf("\n\033[34m%s\033[0m\n", content);
            strcpy(agent.messages[agent.msg_count].role, "assistant");
            strncpy(agent.messages[agent.msg_count].content, content, MAX_CONTENT-1);
            agent.msg_count++;
        } else {
            printf("\033[90m[调试] AI返回了空内容\033[0m\n");
        }
    } else {
        printf("\033[90m[调试] 无法从响应中提取content字段\033[0m\n");
        // 打印响应的前200个字符用于调试
        char preview[201];
        strncpy(preview, resp, 200);
        preview[200] = '\0';
        printf("\033[90m响应预览: %s...\033[0m\n", preview);
    }

    return 0;
}
