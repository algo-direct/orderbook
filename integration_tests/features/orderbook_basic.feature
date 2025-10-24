Feature: Order Book receives snapshot

  @SmokeTest
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
    And Simulator process any pending snapshot request from Order Book
    When Get snapshot from Order Book
    Then Verify Order Book sequence number is 18
    And Bids in the snapshot contains level 3988.57 with size 20
    And Bids in the snapshot does not contains level 3988.50

  @ReconnectTest
  Scenario: Order Book recovers correctly from websocket disconnect
    Given Order Book Simulator is running
    And Order Book is running
    And Verify Order Book is connected to simulator via websocket
    And Set following snapshot in simulator
      """
      {
        "sequence": "16",
        "asks":[
          ["3988.59","3"],
          ["3988.60","47"],
          ["3988.61","32"],
          ["3988.62","8"]
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
    And Simulator sends incremental update to Order Book
    And Simulator process any pending snapshot request from Order Book
    And Simulator disconnect Order Book's websocket connection
    # After websocket disconnet Order Book immidiatly connects to simulator again 
    # and sends websocket subscription and http GET snapshot request,
    # On snapshot request, simulator copy the snapshot and hold on responding until signaled by this test
    And Add bid at level 3988.58 and size 27
    And Simulator sends incremental update to Order Book
    And Remove bid from level 3988.50
    And Add ask at level 3988.63 and size 14
    And Remove ask from level 3988.60
    And Verify Order Book is connected to simulator via websocket
    And Simulator sends incremental update to Order Book
    # Above four steps made sure that simulator gets 3 incremental updates, still waiting for snapshot
    And Simulator process any pending snapshot request from Order Book
    # Adove step signals simulator to respond on snapshot request sent by Order Book previously
    # And responded snapshot is stale, so Order Book must apply incremental updates on it
    When Get snapshot from Order Book
    Then Verify Order Book sequence number is 21
    And Bids in the snapshot contains level 3988.57 with size 20
    And Bids in the snapshot contains level 3988.58 with size 27
    And Bids in the snapshot does not contains level 3988.50
    And Asks in the snapshot contains level 3988.63 with size 14
    And Asks in the snapshot does not contains level 3988.60
