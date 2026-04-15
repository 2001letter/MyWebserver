// auth.js - 登录/注册页面（支持模拟模式，确保前端演示正常）

(function() {
    // 检查是否已登录，若已登录则跳转首页
    if (typeof isLoggedIn === 'function' && isLoggedIn()) {
        window.location.href = 'index.html';
        return;
    }

    const urlParams = new URLSearchParams(window.location.search);
    const tabParam = urlParams.get('tab');
    const loginPanel = document.getElementById('login-panel');
    const registerPanel = document.getElementById('register-panel');
    const tabLoginBtn = document.getElementById('tab-login');
    const tabRegisterBtn = document.getElementById('tab-register');
    const authMessage = document.getElementById('auth-message');

    function showMessage(msg, isError = true) {
        if (authMessage) {
            authMessage.textContent = msg;
            authMessage.style.color = isError ? '#e53e3e' : '#10b981';
            setTimeout(() => {
                if (authMessage) authMessage.textContent = '';
            }, 5000);
        } else {
            alert(msg);
        }
    }

    function setActiveTab(tab) {
        if (tab === 'login') {
            tabLoginBtn.classList.add('active');
            tabRegisterBtn.classList.remove('active');
            loginPanel.style.display = 'block';
            registerPanel.style.display = 'none';
        } else {
            tabRegisterBtn.classList.add('active');
            tabLoginBtn.classList.remove('active');
            loginPanel.style.display = 'none';
            registerPanel.style.display = 'block';
        }
    }

    if (tabParam === 'register') {
        setActiveTab('register');
    } else {
        setActiveTab('login');
    }

    tabLoginBtn.addEventListener('click', () => setActiveTab('login'));
    tabRegisterBtn.addEventListener('click', () => setActiveTab('register'));

    // 模拟模式开关（当后端不可用时自动启用，也可强制设为 true）
    const USE_MOCK = false;  // 改为 false 则使用真实后端

    // 登录逻辑
    document.getElementById('do-login').addEventListener('click', async () => {
        const username = document.getElementById('login-username').value.trim();
        const password = document.getElementById('login-password').value;

        if (!username) return showMessage('请输入用户名');
        if (!password) return showMessage('请输入密码');

        const loginBtn = document.getElementById('do-login');
        const originalText = loginBtn.textContent;
        loginBtn.disabled = true;
        loginBtn.textContent = '登录中...';

        if (USE_MOCK) {
            // 模拟登录成功
            setTimeout(() => {
                localStorage.setItem('nebula_user', JSON.stringify({
                    username: username,
                    token: 'mock-token-' + Date.now()
                }));
                showMessage(`欢迎回来，${username}！`, false);
                setTimeout(() => {
                    window.location.href = 'index.html';
                }, 1000);
                loginBtn.disabled = false;
                loginBtn.textContent = originalText;
            }, 500);
            return;
        }

        // 真实后端请求（保留）
        try {
            const response = await fetch('/api/login', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });
            const data = await response.json();
            if (response.ok && data.code === 200) {
                localStorage.setItem('nebula_user', JSON.stringify({
                    username: data.data.username,
                    token: data.data.token
                }));
                showMessage('登录成功！正在跳转...', false);
                setTimeout(() => {
                    window.location.href = 'index.html';
                }, 1000);
            } else {
                showMessage(data.message || '登录失败，请检查用户名或密码');
            }
        } catch (error) {
            console.error('登录请求失败:', error);
            showMessage('网络错误，无法连接服务器（您可开启模拟模式）');
        } finally {
            loginBtn.disabled = false;
            loginBtn.textContent = originalText;
        }
    });

    // 注册逻辑
    document.getElementById('do-register').addEventListener('click', async () => {
        const username = document.getElementById('reg-username').value.trim();
        const email = document.getElementById('reg-email').value.trim();
        const pwd = document.getElementById('reg-password').value;
        const confirm = document.getElementById('reg-confirm').value;

        if (username.length < 3) return showMessage('用户名至少需要3个字符');
        if (!email.includes('@')) return showMessage('请填写有效的邮箱地址');
        if (pwd.length < 4) return showMessage('密码长度至少4位');
        if (pwd !== confirm) return showMessage('两次输入的密码不一致');

        const regBtn = document.getElementById('do-register');
        const originalText = regBtn.textContent;
        regBtn.disabled = true;
        regBtn.textContent = '注册中...';

        if (USE_MOCK) {
            setTimeout(() => {
                localStorage.setItem('nebula_user', JSON.stringify({
                    username: username,
                    token: 'mock-token-' + Date.now()
                }));
                showMessage(`注册成功，欢迎 ${username}！`, false);
                setTimeout(() => {
                    window.location.href = 'index.html';
                }, 1000);
                regBtn.disabled = false;
                regBtn.textContent = originalText;
            }, 500);
            return;
        }

        try {
            const response = await fetch('/api/register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, email, password: pwd })
            });
            const data = await response.json();
            if (response.ok && data.code === 200) {
                localStorage.setItem('nebula_user', JSON.stringify({
                    username: data.data.username,
                    token: data.data.token
                }));
                showMessage('注册成功！正在跳转...', false);
                setTimeout(() => {
                    window.location.href = 'index.html';
                }, 1000);
            } else if(data.code === 400){
                showMessage(data.message || '用户已存在');
            } else {
                showMessage(data.message || '注册失败，请稍后再试');
            }
        } catch (error) {
            console.error('注册请求失败:', error);
            showMessage('网络错误，无法连接服务器（您可开启模拟模式）');
        } finally {
            regBtn.disabled = false;
            regBtn.textContent = originalText;
        }
    });
})();