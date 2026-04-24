import unittest

from helpers.actor_name_manager import ActorNameManager


class FakeUnrealConnection:
    def __init__(self, actors):
        self.actors = actors

    def send_command(self, command, params=None):
        self.last_command = command
        self.last_params = params or {}
        return {
            "status": "success",
            "result": {
                "actors": [{"name": actor_name} for actor_name in self.actors]
            },
        }


class ActorNameManagerTests(unittest.TestCase):
    def test_detects_wrapped_actor_response(self):
        manager = ActorNameManager()

        self.assertTrue(manager._actor_exists("Cube", FakeUnrealConnection(["Cube"])))

    def test_generates_unique_name_when_base_exists(self):
        manager = ActorNameManager()

        unique_name = manager.generate_unique_name("Cube", FakeUnrealConnection(["Cube"]))

        self.assertNotEqual(unique_name, "Cube")
        self.assertTrue(unique_name.startswith("Cube_"))


if __name__ == "__main__":
    unittest.main()
