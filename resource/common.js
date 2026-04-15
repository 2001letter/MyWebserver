// common.js - 所有页面共用的脚本
// 登录状态管理与导航栏动态渲染

function getUserInfo() {
    const userStr = localStorage.getItem('nebula_user');
    if (userStr) {
        try {
            return JSON.parse(userStr);
        } catch(e) {
            return null;
        }
    }
    return null;
}

function isLoggedIn() {
    return !!getUserInfo();
}

function logout() {
    localStorage.removeItem('nebula_user');
    window.location.href = 'index.html';
}

function updateNavBar() {
    const navContainer = document.querySelector('.nav-buttons');
    if (!navContainer) {
        console.warn('未找到 .nav-buttons 容器');
        return;
    }
    console.log('✅ 导航栏容器已找到');

    const loginBtn = navContainer.querySelector('a.btn-login');
    const registerBtn = navContainer.querySelector('a.btn-register');
    console.log('登录按钮:', loginBtn, '注册按钮:', registerBtn);

    // 移除已添加的用户区域
    const existingUserArea = navContainer.querySelector('.user-area');
    if (existingUserArea) existingUserArea.remove();

    const user = getUserInfo();
    console.log('当前用户信息:', user);

    if (user && user.username) {
        // 已登录：隐藏登录/注册按钮
        if (loginBtn) loginBtn.style.display = 'none';
        if (registerBtn) registerBtn.style.display = 'none';

        // 创建用户区域
        const userArea = document.createElement('div');
        userArea.className = 'user-area';
        userArea.innerHTML = `
            <span class="username-display">👤 ${escapeHtml(user.username)}</span>
            <button class="logout-btn">🚪 退出</button>
        `;
        navContainer.appendChild(userArea);
        console.log('✅ 用户区域已添加到导航栏');

        // 绑定退出事件
        const logoutBtn = userArea.querySelector('.logout-btn');
        logoutBtn.addEventListener('click', (e) => {
            e.preventDefault();
            logout();
        });
    } else {
        // 未登录：确保登录/注册按钮可见
        if (loginBtn) loginBtn.style.display = 'inline-flex';
        if (registerBtn) registerBtn.style.display = 'inline-flex';
        console.log('未登录状态，显示登录/注册按钮');
    }
}

function escapeHtml(str) {
    if (!str) return '';
    return str.replace(/[&<>]/g, function(m) {
        if (m === '&') return '&amp;';
        if (m === '<') return '&lt;';
        if (m === '>') return '&gt;';
        return m;
    });
}

function updateLiveTime() {
    const timeSpan = document.getElementById('live-time');
    if (timeSpan) {
        timeSpan.innerText = new Date().toLocaleTimeString('zh-CN', { hour12: false });
    }
}
updateLiveTime();
setInterval(updateLiveTime, 1000);

function highlightCurrentNav() {
    const currentPath = window.location.pathname.split('/').pop() || 'index.html';
    const navLinks = document.querySelectorAll('.nav-btn');
    navLinks.forEach(link => {
        const href = link.getAttribute('href');
        if (href === currentPath) {
            link.style.background = 'white';
            link.style.boxShadow = '0 2px 8px rgba(0,0,0,0.1)';
        }
    });
}

document.addEventListener('DOMContentLoaded', function() {
    updateNavBar();
    highlightCurrentNav(); // 按需开启
});