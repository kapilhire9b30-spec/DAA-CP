package edu.campus.alloc.dto;

import com.fasterxml.jackson.annotation.JsonInclude;

@JsonInclude(JsonInclude.Include.NON_NULL)
public class ApiResponse<T> {
    private boolean ok;
    private String error;
    private String warning;
    private T data;

    public ApiResponse() {
    }

    public ApiResponse(boolean ok, String error, String warning, T data) {
        this.ok = ok;
        this.error = error;
        this.warning = warning;
        this.data = data;
    }

    public static <T> ApiResponse<T> success(T data) {
        return new ApiResponse<>(true, null, null, data);
    }

    public static <T> ApiResponse<T> successWithWarning(T data, String warning) {
        return new ApiResponse<>(true, null, warning, data);
    }

    public static <T> ApiResponse<T> failure(String error) {
        return new ApiResponse<>(false, error, null, null);
    }

    public boolean isOk() {
        return ok;
    }

    public void setOk(boolean ok) {
        this.ok = ok;
    }

    public String getError() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    public String getWarning() {
        return warning;
    }

    public void setWarning(String warning) {
        this.warning = warning;
    }

    public T getData() {
        return data;
    }

    public void setData(T data) {
        this.data = data;
    }
}
