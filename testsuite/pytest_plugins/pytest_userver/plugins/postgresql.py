"""
Plugin that imports the required fixtures to start the database
and adjusts the PostgreSQL "dbconnection" static config value.
"""

import typing

import pytest

from pytest_userver import sql


pytest_plugins = [
    'testsuite.databases.pgsql.pytest_plugin',
    'pytest_userver.plugins.core',
]


USERVER_CONFIG_HOOKS = ['userver_pg_config']


@pytest.fixture(scope='session')
def userver_pg_config(pgsql_local):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets the `dbconnection` to the testsuite started PostgreSQL credentials
    if there's only one `dbconnection` in static config.

    @ingroup userver_testsuite_fixtures
    """

    if not pgsql_local:
        raise ValueError(
            'Override the "pgsql_local" fixture so that testsuite knowns how '
            'to start the PostgreSQL database',
        )

    if len(pgsql_local) > 1:
        raise ValueError(
            f'Found more than one entry in "pgsql_local": '
            f'{list(pgsql_local.keys())}. '
            f'The "userver_pg_config" fixture supports '
            f'only one entry in "pgsql_local" fixture. The '
            f'"userver_pg_config" fixture should be overridden and '
            f'the "dbconnection" for the components::Postgres '
            f'components should be adjusted via the overridden fixture.',
        )

    uri = list(pgsql_local.values())[0].get_uri()

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        postgre_dbs = {
            name: params
            for name, params in components.items()
            if params
            and ('dbconnection' in params or 'dbconnection#env' in params)
        }

        if len(postgre_dbs) > 1:
            raise ValueError(
                f'Found more than one components with "dbconnection": '
                f'{list(postgre_dbs.keys())}. '
                f'The "userver_pg_config" fixture should be overridden and '
                f'the "dbconnection" for the components::Postgres '
                f'components should be adjusted via the overridden fixture.',
            )

        for config in postgre_dbs.values():
            config['dbconnection'] = uri
            config.pop('dbalias', None)

    return _patch_config


@pytest.fixture
def userver_pg_trx(
        testpoint,
) -> typing.Generator[sql.RegisteredTrx, None, None]:
    """
    The fixture maintains transaction fault injection state using
    RegisteredTrx class.

    @see pytest_userver.sql.RegisteredTrx

    @snippet postgresql/functional_tests/integration_tests/tests/test_trx_failure.py  fault injection

    @ingroup userver_testsuite_fixtures
    """  # noqa: E501

    registered = sql.RegisteredTrx()

    @testpoint('pg_trx_commit')
    def _pg_trx_tp(data):
        should_fail = registered.is_failure_enabled(data['trx_name'])
        return {'trx_should_fail': should_fail}

    yield registered
