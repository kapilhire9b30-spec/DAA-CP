package edu.campus.alloc.auth;

import edu.campus.alloc.entity.UserRole;

import java.lang.annotation.*;

@Target({ElementType.METHOD, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@Documented
public @interface RequireRole {
    UserRole value();
}
