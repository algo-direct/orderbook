# Demo Url https://invaeg.com/orderbook/
![Demo](order_booker_web_content/img/WebView.gif)


# Design overview
![Design overview](order_booker_web_content/img/orderbook.svg)

---

# Integration Tests (Cucumber-style testing using python `behave`)
![integration_tests](order_booker_web_content/img/integration_tests.png)


# How to run intergration tests
`docker compose run --rm intergration_tests`

# How to run build
`docker compose build`

# How to edit code in `vscode`
* Clone git repo
* Open repo in vscode
* Press `Ctrl + Shift + P` and then type `dev open` and choose `Dev Containers: Open Folder in Container...`
    * <img src="order_booker_web_content/img/open_in_dev_c.png" width="600"/>
* VS Code will open in a docker container and code can be built or edited in-place
