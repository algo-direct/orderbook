from behave import given, when, then

@given('the user is on the login page')
def step_impl(context):
    # Simulate navigating to the login page
    context.current_page = 'login_page'
    print("User is on the login page")

@when('the user enters "{username}" and "{password}"')
def step_impl(context, username, password):
    context.username = username
    context.password = password
    print(f"User enters username: {username} and password: {password}")

@when('clicks the login button')
def step_impl(context):
    # Simulate clicking the login button and checking credentials
    if context.username == "username" and context.password == "password":
        context.logged_in = True
        print("Login button clicked - credentials valid")
    else:
        context.logged_in = False
        print("Login button clicked - credentials invalid")

@then('the user should be redirected to the dashboard')
def step_impl(context):
    assert context.logged_in is True, "User was not redirected to the dashboard"
    context.current_page = 'dashboard'
    print("User redirected to the dashboard")

@then('an error message "{message}" should be displayed')
def step_impl(context, message):
    assert context.logged_in is False, "User was unexpectedly logged in"
    # Simulate checking for the error message
    displayed_message = "Invalid credentials" # This would come from your application
    assert displayed_message == message, f"Expected '{message}' but got '{displayed_message}'"
    print(f"Error message '{message}' displayed")