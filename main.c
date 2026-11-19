#ifndef KPI_H
#define KPI_H

#include <stdbool.h>

#define MAX_NAME_LEN 64
#define MAX_DEPT_LEN 32
#define INITIAL_LOG_CAP 4

// Thresholds for flagging underperformance
#define MIN_PRODUCTIVITY_THRESHOLD 60.0
#define MIN_RELIABILITY_THRESHOLD  75.0
#define MAX_ERROR_RATE_THRESHOLD   15.0

typedef struct {
    int task_id;
    int day_stamp;        // Sequential day number (e.g., 1 to 365) for time filtering
    double hours_spent;
    double quality_score; // 0.0 to 100.0
    bool has_error;       // Derived or input flag (e.g., quality < 70 or explicit error)
} TaskLog;

typedef struct {
    int emp_id;
    char name[MAX_NAME_LEN];
    char department[MAX_DEPT_LEN];
    TaskLog *logs;        // Dynamic array of task logs
    int log_count;
    int log_capacity;

    // Computed KPIs
    double avg_completion_time;
    double error_rate;
    double productivity_score;
    double reliability_index;
    bool is_flagged;
} Employee;

typedef struct {
    double avg_productivity;
    double median_productivity;
    double avg_reliability;
    double median_reliability;
    int total_tasks;
    int flagged_count;
} SummaryReport;

// Memory & Data Management (storage.c)
Employee* create_employee_list(int *capacity);
void add_employee(Employee **list, int *count, int *capacity, int id, const char *name, const char *dept);
bool add_task_log(Employee *emp, int task_id, int day_stamp, double hours, double quality, bool has_error);
Employee* search_employee_by_id(Employee *list, int count, int emp_id);
void sort_employees_by_score(Employee *list, int count);
void free_all_employees(Employee *list, int count);
void load_sample_data(Employee **list, int *count, int *capacity);

// KPI Computation & Analytics (kpi.c)
double compute_avg_completion_time(const Employee *emp, int start_day, int end_day);
double compute_error_rate(const Employee *emp, int start_day, int end_day);
double compute_productivity_score(const Employee *emp, int start_day, int end_day);
double compute_reliability_index(const Employee *emp, int start_day, int end_day);
void update_employee_kpis(Employee *emp, int start_day, int end_day);
const char* detect_performance_trend(const Employee *emp, int last_n);
SummaryReport generate_summary_report(Employee *list, int count, const char *dept_filter, int start_day, int end_day);

#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kpi.h"

static int compare_doubles(const void *a, const void *b) {
    double diff = (*(const double *)a - *(const double *)b);
    return (diff > 0) - (diff < 0);
}

static double calculate_median(double *arr, int n) {
    if (n <= 0) return 0.0;
    qsort(arr, n, sizeof(double), compare_doubles);
    if (n % 2 == 1) {
        return arr[n / 2];
    }
    return (arr[(n / 2) - 1] + arr[n / 2]) / 2.0;
}

double compute_avg_completion_time(const Employee *emp, int start_day, int end_day) {
    double total_hours = 0.0;
    int valid_tasks = 0;

    for (int i = 0; i < emp->log_count; i++) {
        if (emp->logs[i].day_stamp >= start_day && emp->logs[i].day_stamp <= end_day) {
            total_hours += emp->logs[i].hours_spent;
            valid_tasks++;
        }
    }
    return (valid_tasks > 0) ? (total_hours / valid_tasks) : 0.0;
}

double compute_error_rate(const Employee *emp, int start_day, int end_day) {
    int error_tasks = 0;
    int valid_tasks = 0;

    for (int i = 0; i < emp->log_count; i++) {
        if (emp->logs[i].day_stamp >= start_day && emp->logs[i].day_stamp <= end_day) {
            if (emp->logs[i].has_error) {
                error_tasks++;
            }
            valid_tasks++;
        }
    }
    return (valid_tasks > 0) ? ((double)error_tasks / valid_tasks) * 100.0 : 0.0;
}

// Productivity = (Average Quality * Task Velocity Factor) capped at 100
double compute_productivity_score(const Employee *emp, int start_day, int end_day) {
    double total_quality = 0.0;
    double total_hours = 0.0;
    int valid_tasks = 0;

    for (int i = 0; i < emp->log_count; i++) {
        if (emp->logs[i].day_stamp >= start_day && emp->logs[i].day_stamp <= end_day) {
            total_quality += emp->logs[i].quality_score;
            total_hours += emp->logs[i].hours_spent;
            valid_tasks++;
        }
    }
    if (valid_tasks == 0 || total_hours <= 0.0) return 0.0;

    double avg_quality = total_quality / valid_tasks;
    double tasks_per_8hr_day = (valid_tasks / total_hours) * 8.0;
    double velocity_multiplier = (tasks_per_8hr_day > 1.2) ? 1.2 : tasks_per_8hr_day;

    double score = avg_quality * velocity_multiplier;
    return (score > 100.0) ? 100.0 : score;
}

// Reliability = 0.6 * (100 - Error Rate) + 0.4 * Average Quality
double compute_reliability_index(const Employee *emp, int start_day, int end_day) {
    double total_quality = 0.0;
    int valid_tasks = 0;

    for (int i = 0; i < emp->log_count; i++) {
        if (emp->logs[i].day_stamp >= start_day && emp->logs[i].day_stamp <= end_day) {
            total_quality += emp->logs[i].quality_score;
            valid_tasks++;
        }
    }
    if (valid_tasks == 0) return 0.0;

    double avg_quality = total_quality / valid_tasks;
    double err_rate = compute_error_rate(emp, start_day, end_day);

    return (0.6 * (100.0 - err_rate)) + (0.4 * avg_quality);
}

void update_employee_kpis(Employee *emp, int start_day, int end_day) {
    emp->avg_completion_time = compute_avg_completion_time(emp, start_day, end_day);
    emp->error_rate = compute_error_rate(emp, start_day, end_day);
    emp->productivity_score = compute_productivity_score(emp, start_day, end_day);
    emp->reliability_index = compute_reliability_index(emp, start_day, end_day);

    emp->is_flagged = (emp->productivity_score < MIN_PRODUCTIVITY_THRESHOLD) ||
                      (emp->reliability_index < MIN_RELIABILITY_THRESHOLD) ||
                      (emp->error_rate > MAX_ERROR_RATE_THRESHOLD);
}

// Compares average quality of second half vs first half of last N records
const char* detect_performance_trend(const Employee *emp, int last_n) {
    if (emp->log_count < 2 || last_n < 2) {
        return "Insufficient Data";
    }
    int n = (emp->log_count < last_n) ? emp->log_count : last_n;
    int start_idx = emp->log_count - n;
    int mid = n / 2;

    double first_half_sum = 0.0, second_half_sum = 0.0;
    for (int i = 0; i < mid; i++) {
        first_half_sum += emp->logs[start_idx + i].quality_score;
    }
    for (int i = mid; i < n; i++) {
        second_half_sum += emp->logs[start_idx + i].quality_score;
    }

    double first_avg = first_half_sum / mid;
    double second_avg = second_half_sum / (n - mid);
    double delta = second_avg - first_avg;

    if (delta >= 3.0) return "Improving (^)";
    if (delta <= -3.0) return "Worsening (v)";
    return "Stable (-)";
}

SummaryReport generate_summary_report(Employee *list, int count, const char *dept_filter, int start_day, int end_day) {
    SummaryReport report = {0};
    if (count == 0) return report;

    double *prod_scores = malloc(count * sizeof(double));
    double *rel_scores = malloc(count * sizeof(double));
    int matched = 0;

    for (int i = 0; i < count; i++) {
        if (dept_filter != NULL && strlen(dept_filter) > 0 &&
            strcmp(dept_filter, "ALL") != 0 &&
            strcmp(list[i].department, dept_filter) != 0) {
            continue;
        }

        update_employee_kpis(&list[i], start_day, end_day);

        int tasks_in_range = 0;
        for (int j = 0; j < list[i].log_count; j++) {
            if (list[i].logs[j].day_stamp >= start_day && list[i].logs[j].day_stamp <= end_day) {
                tasks_in_range++;
            }
        }

        prod_scores[matched] = list[i].productivity_score;
        rel_scores[matched] = list[i].reliability_index;
        report.avg_productivity += list[i].productivity_score;
        report.avg_reliability += list[i].reliability_index;
        report.total_tasks += tasks_in_range;
        if (list[i].is_flagged) report.flagged_count++;
        matched++;
    }

    if (matched > 0) {
        report.avg_productivity /= matched;
        report.avg_reliability /= matched;
        report.median_productivity = calculate_median(prod_scores, matched);
        report.median_reliability = calculate_median(rel_scores, matched);
    }

    free(prod_scores);
    free(rel_scores);
    return report;
}
