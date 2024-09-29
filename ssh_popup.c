#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>

#define MAX_COMMAND_LENGTH 256
#define SSH_LIST_FILE_PATH "/home/klein/codeWS/C/ssh_gui/ssh_list.txt"
#define PRIVATE_KEY_PATH "/home/klein/.ssh/id_rsa"

// Function to get local IP address
void get_local_ip(char *ip_buffer) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            if (strcmp(ifa->ifa_name, "lo") != 0) {
                strncpy(ip_buffer, host, NI_MAXHOST);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

// Function to get public IP address
void get_public_ip(char *ip_buffer) {
    FILE *fp;
    char path[1035];

    fp = popen("curl -s ifconfig.me", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        strncpy(ip_buffer, path, 1035);
    }

    pclose(fp);
}

// Function to get SSID using nmcli
void get_ssid(char *ssid_buffer) {
    FILE *fp;
    char path[1035];

    fp = popen("nmcli -t -f active,ssid dev wifi | egrep '^yes' | cut -d: -f2", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        strncpy(ssid_buffer, path, 1035);
    }

    pclose(fp);
}

// Function to get SSH service status
void get_ssh_service_status(char *ssh_service_status) {
    FILE *fp;
    char path[1035];

    fp = popen("systemctl is-active ssh", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        strncpy(ssh_service_status, path, 1035);
    }

    pclose(fp);
}

// Function to check if port 22 is open and display relevant SSH information
void get_port_22_status(char *port_22_status) {
    FILE *fp;
    char path[1035];

    fp = popen("ss -tunlp | grep ':22 '", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    // Clear the buffer before appending
    port_22_status[0] = '\0';

    while (fgets(path, sizeof(path) - 1, fp) != NULL) {
        // Append the relevant lines to the status buffer
        strcat(port_22_status, path);
    }

    pclose(fp);
}

// Function to get active connections using ss
void get_active_connections(char *conn_buffer) {
    FILE *fp;
    char path[1035];

    fp = popen("ss -tunap", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    while (fgets(path, sizeof(path) - 1, fp) != NULL) {
        strcat(conn_buffer, path);
    }

    pclose(fp);
}

// Function to execute SSH command
void execute_ssh(GtkWidget *widget, gpointer data) {
    const char *command = (const char *)data;

    char full_command[MAX_COMMAND_LENGTH];
    snprintf(full_command, sizeof(full_command), "ssh -i %s %s", PRIVATE_KEY_PATH, command);

    printf("Executing command: %s\n", full_command); // Debugging output

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        printf("Attempting to execute alacritty with command: %s\n", full_command);
        execlp("alacritty", "alacritty", "-e", "bash", "-c", full_command, NULL);
        printf("alacritty failed, attempting to execute st with command: %s\n", full_command);
        execlp("st", "st", "-e", "bash", "-c", full_command, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        // Parent process
        gtk_main_quit(); // Close the GTK main loop
    }
}

// Function to create buttons for each SSH command
void create_buttons(GtkWidget *box) {
    FILE *file = fopen(SSH_LIST_FILE_PATH, "r");
    if (!file) {
        perror("Error opening file");
        printf("Path attempted: %s\n", SSH_LIST_FILE_PATH); // Debugging output
        return;
    }

    char line[MAX_COMMAND_LENGTH];
    GtkWidget *prev_button = NULL;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character from the line
        line[strcspn(line, "\n")] = 0;

        // Find the first space to locate the start of the SSH command
        char *command = strchr(line, ' ');
        if (command && strlen(command) > 1) {
            command++; // Skip the space to get the actual command
            GtkWidget *button = gtk_button_new_with_label(command);
            gtk_widget_set_name(button, "ssh-button"); // Set name for CSS
            gtk_widget_set_can_focus(button, TRUE);
            g_signal_connect(button, "clicked", G_CALLBACK(execute_ssh), g_strdup(command));
            gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        }
    }

    fclose(file);
}

// Function to open SSH instructions in Neovim
void open_ssh_instructions(GtkWidget *widget, gpointer data) {
    const char *file_path = (const char *)data;

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execlp("alacritty", "alacritty", "-e", "nvim", file_path, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        // Parent process
        gtk_main_quit(); // Close the GTK main loop
    }
}

// Key press event handler for buttons
gboolean on_button_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    GtkWidget *parent = gtk_widget_get_parent(widget);
    GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
    GList *current = g_list_find(children, widget);

    switch (event->keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_Left:
            if (current->prev) {
                gtk_widget_grab_focus(GTK_WIDGET(current->prev->data));
            }
            break;
        case GDK_KEY_Down:
        case GDK_KEY_Right:
            if (current->next) {
                gtk_widget_grab_focus(GTK_WIDGET(current->next->data));
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

// Key press event handler for window
gboolean on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_q || event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_Delete) {
        gtk_main_quit();
        return TRUE;
    }
    return FALSE;
}

void apply_css(GtkWidget *widget, GtkStyleProvider *provider) {
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget), provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
    if (GTK_IS_CONTAINER(widget)) {
        gtk_container_forall(GTK_CONTAINER(widget), (GtkCallback)apply_css, provider);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Get system info
    char local_ip[NI_MAXHOST] = {0};
    char public_ip[1035] = {0};
    char ssid[1035] = {0};
    char username[256] = {0};
    char ssh_service_status[1035] = {0};
    char port_22_status[2048] = {0};
    char active_connections[2048] = {0};
    char info[512]; // Declare the info variable

    get_local_ip(local_ip);
    get_public_ip(public_ip);
    get_ssid(ssid);
    getlogin_r(username, sizeof(username));
    get_ssh_service_status(ssh_service_status);
    get_port_22_status(port_22_status);
    get_active_connections(active_connections);

    // Create CSS provider and load CSS
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider,
        "* { font-family: 'Arial'; }\n"
        "#app-title { font-size: 24px; font-weight: bold; color: #00FF00; }\n"
        "#info-label { font-size: 14px; color: #FFFFFF; }\n"
        "#ssh-button { font-size: 14px; background-color: #1E1E1E; color: #000000; border-radius: 4px; padding: 5px 10px; }\n"
        "#ssh-button:hover { background-color: #333333; }\n"
        "window { background-color: #2E2E2E; }\n",
        -1, NULL);

    // Create GTK window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "SSH Commands");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_window_key_press), NULL);

    // Create a horizontal box to organize sections
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    // Create left box for SSH and firewall info
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), left_box, FALSE, FALSE, 10);

    // Add SSH service status to left box
    GtkWidget *label = gtk_label_new("SSH Service Status:");
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(left_box), label, FALSE, FALSE, 5);

    char ssh_status_markup[512];
    if (strncmp(ssh_service_status, "active", 6) == 0) {
        snprintf(ssh_status_markup, sizeof(ssh_status_markup), "<span foreground='green'>Active ✓</span>");
    } else {
        snprintf(ssh_status_markup, sizeof(ssh_status_markup), "<span foreground='red'>Inactive ✗</span>");
    }

    GtkWidget *ssh_service_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ssh_service_label), ssh_status_markup);
    gtk_widget_set_name(ssh_service_label, "info-label");
    gtk_box_pack_start(GTK_BOX(left_box), ssh_service_label, FALSE, FALSE, 5);

    label = gtk_label_new("Port 22 Status:");
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(left_box), label, FALSE, FALSE, 5);

    GtkWidget *port_22_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(port_22_text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(port_22_text_view), FALSE);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(port_22_text_view));
    gtk_text_buffer_set_text(buffer, port_22_status, -1);
    gtk_box_pack_start(GTK_BOX(left_box), port_22_text_view, TRUE, TRUE, 5);

    // Create center box for user info and buttons
    GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), center_box, TRUE, TRUE, 10);

    // Add application title to center box
    label = gtk_label_new("SSH Command Center");
    gtk_widget_set_name(label, "app-title");
    gtk_box_pack_start(GTK_BOX(center_box), label, FALSE, FALSE, 10);

    // Add system info labels to center box
    snprintf(info, sizeof(info), "Username: %s", username);
    label = gtk_label_new(info);
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(center_box), label, FALSE, FALSE, 5);

    snprintf(info, sizeof(info), "Local IP: %s", local_ip);
    label = gtk_label_new(info);
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(center_box), label, FALSE, FALSE, 5);

    snprintf(info, sizeof(info), "Public IP: %s", public_ip);
    label = gtk_label_new(info);
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(center_box), label, FALSE, FALSE, 5);

    snprintf(info, sizeof(info), "SSID: %s", ssid);
    label = gtk_label_new(info);
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(center_box), label, FALSE, FALSE, 5);

    // Create buttons for SSH commands
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    create_buttons(button_box);
    gtk_box_pack_start(GTK_BOX(center_box), button_box, FALSE, FALSE, 5);

    // Add button to open SSH instructions in Neovim
    GtkWidget *edit_button = gtk_button_new_with_label("Edit SSH Instructions");
    gtk_widget_set_name(edit_button, "ssh-button");
    gtk_widget_set_can_focus(edit_button, TRUE);
    g_signal_connect(edit_button, "clicked", G_CALLBACK(open_ssh_instructions), (gpointer)SSH_LIST_FILE_PATH);
    gtk_box_pack_start(GTK_BOX(center_box), edit_button, FALSE, FALSE, 5);

    // Create right box for active connections
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), right_box, TRUE, TRUE, 10);

    // Add active connections to right box
    label = gtk_label_new("Active Connections:");
    gtk_widget_set_name(label, "info-label");
    gtk_box_pack_start(GTK_BOX(right_box), label, FALSE, FALSE, 5);

    GtkWidget *conn_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(conn_text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(conn_text_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(conn_text_view));
    gtk_text_buffer_set_text(buffer, active_connections, -1);
    gtk_box_pack_start(GTK_BOX(right_box), conn_text_view, TRUE, TRUE, 5);

    // Apply CSS to all widgets
    apply_css(window, GTK_STYLE_PROVIDER(cssProvider));

    // Set up key press event handlers for buttons
    GList *buttons = gtk_container_get_children(GTK_CONTAINER(button_box));
    for (GList *iter = buttons; iter != NULL; iter = iter->next) {
        g_signal_connect(iter->data, "key-press-event", G_CALLBACK(on_button_key_press), NULL);
    }
    g_signal_connect(edit_button, "key-press-event", G_CALLBACK(on_button_key_press), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
