Feature: User Login Functionality

  Scenario: Successful login with valid credentials
    Given the user is on the login page
    When the user enters "username" and "password"
    And clicks the login button
    Then the user should be redirected to the dashboard

  Scenario: Unsuccessful login with invalid credentials
    Given the user is on the login page
    When the user enters "invalid_username" and "wrong_password"
    And clicks the login button
    Then an error message "Invalid credentials" should be displayed