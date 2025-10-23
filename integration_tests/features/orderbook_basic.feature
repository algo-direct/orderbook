Feature: Order Book receives snapshot

  Scenario: On startup Order Book receives snapshot
    Given Order Book Simulator is running
    And Order Book is running
    And Set following snapshot in simulator
      """
      {
        "sequence": "16",
        "asks":[
          ["3988.62","8"],
          ["3988.61","32"],
          ["3988.60","47"],
          ["3988.59","3"]
        ],
        "bids":[
          ["3988.51","56"],
          ["3988.50","15"],
          ["3988.49","100"],
          ["3988.48","10"]
        ]
      }
      """
    And Add bid at level 3988.57 and size 20
    And Remove bid from level 3988.50
    And Simulator sends incremental update to Order Book
    And Simulator process any pending snapshot request
    When Get snapshot from Order Book
    Then Verify Order Book sequence number is 18
    And Bids in the snapshot contains level 3988.57 with size 20
    And Bids in the snapshot does not contains level 3988.50
