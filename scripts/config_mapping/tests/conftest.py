import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--schema-file",
        action="store",
        required=True,
        help="path to the schema file",
    )


@pytest.fixture
def schema_file(request):
    return request.config.getoption("--schema-file")
